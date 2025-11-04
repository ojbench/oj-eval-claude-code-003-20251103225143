#include <bits/stdc++.h>
using namespace std;

/*
 ICPC Management System implementation per /workspace/problem_003/README.md
 - Commands: ADDTEAM, START, SUBMIT, FLUSH, FREEZE, SCROLL, QUERY_RANKING, QUERY_SUBMISSION, END
 - Ranking rules, freeze/scroll mechanics, scoreboard formatting per spec.
 */

struct ProblemState {
    // Pre-freeze attempts and solved flag/time
    int wrong_before = 0; // wrong attempts before first AC (pre-freeze)
    bool solved = false;  // solved (considering only non-frozen problems)
    int solve_time = 0;   // first AC time
    bool attempted_pre_freeze = false; // any attempt before freeze

    // Freeze data
    bool is_frozen = false; // whether this problem is in frozen state
    int frozen_submissions = 0; // submissions after freeze
    int wrong_after_unfreeze = 0; // wrong attempts revealed during scroll (for display)
    bool solved_after_unfreeze = false; // AC revealed during scroll
    int solve_time_after_unfreeze = 0; // AC time after scroll reveal
};

struct SubmissionRec {
    char prob; string status; int time;
};

struct Team {
    string name;
    // per-problem states size M
    vector<ProblemState> probs;
    // submissions (all, ordered by time)
    vector<SubmissionRec> submissions;

    // ranking metrics (current visible state)
    int solved_visible = 0;
    long long penalty_visible = 0;
    vector<int> solve_times_desc; // sorted descending of solve times for solved_visible

    // For frozen tracking: set of frozen problem indices (0-based)
    set<int> frozen_indices; // problems that have y>0 after FREEZE
};

struct SystemState {
    bool started = false;
    int duration = 0;
    int M = 0; // number of problems 'A'.. last

    // map team name -> Team
    unordered_map<string, Team> teams;
    vector<string> team_order_lex; // initial order before first flush

    // freeze status
    bool frozen = false;

    // last flushed rankings (vector of team names in order)
    vector<string> last_scoreboard_order;
};

// Comparison per spec on visible metrics
struct RankCmp {
    unordered_map<string, Team>* tmap;
    bool operator()(const string& a, const string& b) const {
        const Team& A = tmap->at(a);
        const Team& B = tmap->at(b);
        if (A.solved_visible != B.solved_visible) return A.solved_visible > B.solved_visible;
        if (A.penalty_visible != B.penalty_visible) return A.penalty_visible < B.penalty_visible;
        // compare solve_times descending lists: smaller max ranks higher
        const auto &ta = A.solve_times_desc, &tb = B.solve_times_desc;
        size_t na = ta.size(), nb = tb.size();
        size_t n = min(na, nb);
        for (size_t i = 0; i < n; ++i) {
            if (ta[i] != tb[i]) return ta[i] < tb[i]; // smaller max time ranks higher
        }
        if (na != nb) return na > nb; // if different lengths but equal prefix, longer vector means more solves, but solved_visible equal -> lengths equal; keep for safety
        return A.name < B.name;
    }
};

static void recompute_visible(Team &t) {
    t.solved_visible = 0;
    t.penalty_visible = 0;
    t.solve_times_desc.clear();
    for (int i = 0; i < (int)t.probs.size(); ++i) {
        const auto &ps = t.probs[i];
        if (ps.is_frozen) {
            // frozen problems do not contribute until scrolled
            continue;
        }
        if (ps.solved) {
            t.solved_visible++;
            t.penalty_visible += 20LL * ps.wrong_before + ps.solve_time;
            t.solve_times_desc.push_back(ps.solve_time);
        }
    }
    sort(t.solve_times_desc.begin(), t.solve_times_desc.end(), greater<int>());
}

static void flush_scoreboard(SystemState &S) {
    // recompute visible metrics for all teams
    for (auto &kv : S.teams) {
        recompute_visible(kv.second);
    }
    // sort per RankCmp
    vector<string> order;
    order.reserve(S.teams.size());
    for (auto &kv : S.teams) order.push_back(kv.first);
    RankCmp cmp{&S.teams};
    sort(order.begin(), order.end(), cmp);
    S.last_scoreboard_order = order;
}

static void print_scoreboard(const SystemState &S) {
    // Output N lines: team_name ranking solved_count total_penalty A B C ...
    // ranking index is 1-based position in last_scoreboard_order
    unordered_map<string,int> rank_idx;
    for (int i = 0; i < (int)S.last_scoreboard_order.size(); ++i) rank_idx[S.last_scoreboard_order[i]] = i+1;
    for (const string &name : S.last_scoreboard_order) {
        const Team &t = S.teams.at(name);
        cout << t.name << ' ' << rank_idx.at(t.name) << ' ' << t.solved_visible << ' ' << t.penalty_visible;
        // problems A..(A+M-1)
        for (int i = 0; i < S.M; ++i) {
            const auto &ps = t.probs[i];
            cout << ' ';
            if (ps.is_frozen) {
                int x = ps.wrong_before;
                int y = ps.frozen_submissions;
                if (x == 0) cout << "0/" << y;
                else cout << '-' << x << '/' << y;
            } else if (ps.solved) {
                int x = ps.wrong_before;
                if (x == 0) cout << '+';
                else cout << '+' << x;
            } else {
                int x = ps.wrong_before;
                if (!ps.solved) {
                    if (x == 0 && !ps.attempted_pre_freeze) cout << '.';
                    else if (x == 0) cout << "-0"; // display -0 only if attempted but no wrongs
                    else cout << '-' << x;
                }
            }
        }
        cout << '\n';
    }
}

static void unfreeze_one(Team &t, int idx) {
    auto &ps = t.probs[idx];
    // Reveal frozen submissions into pre-freeze accounting: They happened after freeze, but after scrolling, they become visible.
    // Interpret: After scrolling, frozen state lifted; the actual attempts/AC after freeze should be reflected in solved/penalty.
    // We only tracked count y; need to infer wrong/accepted from submissions history.
    // Maintain wrong_after_unfreeze and solved_after_unfreeze by scanning submissions for this team/problem after freeze point.
    ps.is_frozen = false; // unfreezed
    // The wrong_before should already include pre-freeze wrongs; post-freeze wrongs that didn't lead to AC should count towards display '-'?
    // For final scoreboard after scrolling examples, counts like -2 (total wrong including post-freeze?) appear. The spec says display uses x = number of incorrect attempts (not frozen).
    // So after scroll, for unsolved problems, display -total_wrong (pre + post). For solved, display +x where x = wrong before first AC (including both before/after freeze?). But spec earlier defines penalty using X before first correct submission, T time of first correct submission; if AC occurs after freeze, X includes pre-freeze wrongs only; post-freeze wrongs occur before first AC too, but spec: freeze hides info, after scroll we reveal true state; X must include all wrong attempts before first AC irrespective of freeze time.
    // For consistent handling, we will recompute from submissions log when printing after full scroll via recompute_visible after updating per-problem states to include final wrong count and solve time.
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    SystemState S;

    string cmd;
    // Keep a vector of team names order lex for pre-first-flush ranking
    auto recompute_lex_order = [&]() {
        S.team_order_lex.clear();
        for (auto &kv : S.teams) S.team_order_lex.push_back(kv.first);
        sort(S.team_order_lex.begin(), S.team_order_lex.end());
        S.last_scoreboard_order = S.team_order_lex; // before first flush ranking is lex order
    };

    while (cin >> cmd) {
        if (cmd == "ADDTEAM") {
            string name; cin >> name;
            if (S.started) {
                cout << "[Error]Add failed: competition has started.\n";
            } else {
                if (S.teams.count(name)) {
                    cout << "[Error]Add failed: duplicated team name.\n";
                } else {
                    Team t; t.name = name;
                    S.teams.emplace(name, move(t));
                    recompute_lex_order();
                    cout << "[Info]Add successfully.\n";
                }
            }
        } else if (cmd == "START") {
            string tmp; cin >> tmp; // DURATION
            int duration; cin >> duration; cin >> tmp; // PROBLEM
            int pc; cin >> pc;
            if (S.started) {
                cout << "[Error]Start failed: competition has started.\n";
            } else {
                S.started = true; S.duration = duration; S.M = pc;
                for (auto &kv : S.teams) {
                    kv.second.probs.assign(S.M, ProblemState());
                }
                cout << "[Info]Competition starts.\n";
            }
        } else if (cmd == "SUBMIT") {
            char prob; string tmp1, team, tmp2, status, tmp3; int time;
            cin >> prob >> tmp1 >> team >> tmp2 >> status >> tmp3 >> time; // BY team WITH status AT time
            // Record submission
            Team &t = S.teams[team];
            t.submissions.push_back({prob, status, time});
            int idx = prob - 'A';
            auto &ps = t.probs[idx];
            // If frozen status currently active: determine if this problem should be marked as frozen
            if (S.frozen) {
                // If the team hadn't solved this problem before freeze (i.e., ps.solved == false), it becomes frozen upon any submission
                if (!ps.solved) {
                    ps.is_frozen = true;
                    ps.frozen_submissions++;
                    t.frozen_indices.insert(idx);
                    // Track only until first AC during freeze; ignore attempts after AC
                    if (!ps.solved_after_unfreeze) {
                        if (status == "Accepted") {
                            ps.solved_after_unfreeze = true;
                            ps.solve_time_after_unfreeze = time;
                        } else {
                            ps.wrong_after_unfreeze++;
                        }
                    }
                } else {
                    // solved before freeze: post-freeze submissions do not affect scoreboard metrics
                    // Problems solved before freezing are not frozen even if submitted after freezing.
                    // Do nothing.
                }
            } else {
                // Not frozen: update visible states immediately
                ps.attempted_pre_freeze = true;
                if (status == "Accepted") {
                    if (!ps.solved) {
                        ps.solved = true;
                        ps.solve_time = time;
                        // wrong_before already counted
                    }
                } else {
                    if (!ps.solved) {
                        ps.wrong_before++;
                    }
                }
            }
            // No output
        } else if (cmd == "FLUSH") {
            cout << "[Info]Flush scoreboard.\n";
            flush_scoreboard(S);
        } else if (cmd == "FREEZE") {
            if (!S.frozen) {
                S.frozen = true;
                cout << "[Info]Freeze scoreboard.\n";
            } else {
                cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            }
        } else if (cmd == "SCROLL") {
            if (!S.frozen) {
                cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            } else {
                cout << "[Info]Scroll scoreboard.\n";
                // Output scoreboard before scrolling (after flushing)
                flush_scoreboard(S);
                print_scoreboard(S);

                // Perform iterative unfreeze: while any team has frozen_indices non-empty
                // Always pick lowest-ranked team with frozen problems, and smallest problem index
                // After each unfreeze, recompute rankings; if ranking increases (team moves up), and position changed, print a line per each jump? The spec says output each unfreeze that causes a ranking change, one per line, using immediate pair names.
                // We'll detect immediate swap upward steps and print pair: team_name1 (the moving team) team_name2 (the team it overtook), solved_number and penalty_time after update.

                // Build helper to get current order index map
                auto get_rank_idx = [&]() {
                    unordered_map<string,int> r;
                    for (int i = 0; i < (int)S.last_scoreboard_order.size(); ++i) r[S.last_scoreboard_order[i]] = i;
                    return r;
                };

                while (true) {
                    // find lowest-ranked team with frozen problems
                    int chosen_rank = -1;
                    string chosen_team;
                    int chosen_prob = -1;
                    for (int i = (int)S.last_scoreboard_order.size()-1; i >= 0; --i) {
                        const string &name = S.last_scoreboard_order[i];
                        Team &t = S.teams[name];
                        if (!t.frozen_indices.empty()) {
                            chosen_rank = i; chosen_team = name;
                            chosen_prob = *t.frozen_indices.begin();
                            break;
                        }
                    }
                    if (chosen_rank == -1) break; // none

                    Team &t = S.teams[chosen_team];
                    auto &ps = t.probs[chosen_prob];
                    // Apply reveal of frozen submissions for this problem
                    // Consolidate wrong_before and solved based on previously tracked frozen counters
                    if (ps.solved_after_unfreeze) {
                        // first AC time is either pre-freeze solve_time or post-freeze solve time
                        if (!ps.solved) {
                            // AC happened only after freeze; wrong attempts before first AC include pre + post wrongs until that AC
                            ps.solved = true;
                            ps.solve_time = ps.solve_time_after_unfreeze;
                            ps.wrong_before += ps.wrong_after_unfreeze; // include post-freeze wrongs before AC
                        } else {
                            // already solved before freeze; post-freeze submissions ignored
                        }
                    } else {
                        // not solved after reveal; increase wrong_before by post-freeze wrongs
                        ps.wrong_before += ps.wrong_after_unfreeze;
                    }
                    // clear frozen markers for this problem
                    ps.is_frozen = false;
                    ps.frozen_submissions = 0;
                    ps.wrong_after_unfreeze = 0;
                    ps.solved_after_unfreeze = false;
                    ps.solve_time_after_unfreeze = 0;
                    t.frozen_indices.erase(chosen_prob);

                    // recompute rankings
                    auto prev_order = S.last_scoreboard_order;
                    flush_scoreboard(S);
                    // Check if chosen_team moved up compared to prev_order
                    unordered_map<string,int> prev_idx;
                    for (int i = 0; i < (int)prev_order.size(); ++i) prev_idx[prev_order[i]] = i;
                    int old_pos = prev_idx[chosen_team];
                    int new_pos = -1;
                    for (int i = 0; i < (int)S.last_scoreboard_order.size(); ++i) if (S.last_scoreboard_order[i] == chosen_team) new_pos = i;
                    if (new_pos < old_pos) {
                        // It moved up; report the team that previously occupied the new position
                        string replaced = prev_order[new_pos];
                        const Team &tcur = S.teams[chosen_team];
                        cout << chosen_team << ' ' << replaced << ' ' << tcur.solved_visible << ' ' << tcur.penalty_visible << "\n";
                    }
                }
                // After finish, output final scoreboard
                print_scoreboard(S);
                // lift frozen state
                S.frozen = false;
                // Clear any remaining frozen flags that might have y==0
                for (auto &kv : S.teams) {
                    Team &t = kv.second;
                    t.frozen_indices.clear();
                    for (auto &ps : t.probs) {
                        ps.is_frozen = false;
                        ps.frozen_submissions = 0;
                        ps.wrong_after_unfreeze = 0;
                        ps.solved_after_unfreeze = false;
                        ps.solve_time_after_unfreeze = 0;
                    }
                }
            }
        } else if (cmd == "QUERY_RANKING") {
            string name; cin >> name;
            auto it = S.teams.find(name);
            if (it == S.teams.end()) {
                cout << "[Error]Query ranking failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query ranking.\n";
                if (S.frozen) {
                    cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
                }
                // rank after last flush; if never flushed, use lex order
                // Ensure last_scoreboard_order has something
                if (S.last_scoreboard_order.empty()) flush_scoreboard(S);
                int rank = -1;
                for (int i = 0; i < (int)S.last_scoreboard_order.size(); ++i) if (S.last_scoreboard_order[i] == name) { rank = i+1; break; }
                cout << name << " NOW AT RANKING " << rank << "\n";
            }
        } else if (cmd == "QUERY_SUBMISSION") {
            string team, tmp; cin >> team >> tmp; // WHERE
            string cond1; cin >> cond1; // PROBLEM=X
            string cond2; cin >> cond2; // AND
            string cond3; cin >> cond3; // STATUS=Y
            auto it = S.teams.find(team);
            if (it == S.teams.end()) {
                cout << "[Error]Query submission failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query submission.\n";
                // parse PROBLEM= and STATUS=
                char prob_filter; bool prob_all = false;
                if (cond1.rfind("PROBLEM=", 0) == 0) {
                    string val = cond1.substr(8);
                    if (val == "ALL") prob_all = true;
                    else prob_filter = val[0];
                }
                string status_filter; bool status_all = false;
                if (cond3.rfind("STATUS=", 0) == 0) {
                    string val = cond3.substr(7);
                    if (val == "ALL") status_all = true;
                    else status_filter = val;
                }
                // find last submission satisfying
                const auto &subs = it->second.submissions;
                bool found = false; SubmissionRec last{};
                for (int i = (int)subs.size()-1; i >= 0; --i) {
                    const auto &sr = subs[i];
                    if (!prob_all && sr.prob != prob_filter) continue;
                    if (!status_all && sr.status != status_filter) continue;
                    found = true; last = sr; break;
                }
                if (!found) {
                    cout << "Cannot find any submission.\n";
                } else {
                    cout << it->second.name << ' ' << last.prob << ' ' << last.status << ' ' << last.time << "\n";
                }
            }
        } else if (cmd == "END") {
            cout << "[Info]Competition ends.\n";
            break;
        } else {
            // Unknown command, but spec guarantees valid formats
        }
    }
    return 0;
}
