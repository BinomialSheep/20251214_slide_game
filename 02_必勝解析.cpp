#include <bits/stdc++.h>
using namespace std;

constexpr int N = 5;
constexpr int EMPTY = 0;
constexpr int PLAYER1 = 1;
constexpr int PLAYER2 = 2;

constexpr int UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3;
const int dy[] = {-1, 1, 0, 0};
const int dx[] = {0, 0, -1, 1};

struct Action {
  int piece_id;  // board value (P1 positive, P2 negative)
  int direction; // 0..3
};

enum Res { R_WIN, R_LOSE, R_DRAW };

static const char *res_name(Res r) {
  switch (r) {
  case R_WIN:
    return "WIN";
  case R_LOSE:
    return "LOSE";
  case R_DRAW:
    return "DRAW";
  }
  return "?";
}
static const char *dir_name(int d) {
  static const char *s[] = {"UP", "DOWN", "LEFT", "RIGHT"};
  if (0 <= d && d < 4)
    return s[d];
  return "?";
}

class State {
public:
  array<array<int, N>, N> board{};
  int player_id = PLAYER1;

  State() {
    for (int y = 0; y < N; y++)
      for (int x = 0; x < N; x++)
        board[y][x] = EMPTY;
    for (int i = 0; i < N; i++)
      board[0][i] = -(i + 1); // P2
    for (int i = 0; i < N; i++)
      board[N - 1][i] = (i + 1); // P1
    player_id = PLAYER1;
  }

  // winner: 0 none, else PLAYER1/PLAYER2
  int winner() const {
    int c = board[2][2];
    if (c == EMPTY)
      return 0;
    return (c > 0) ? PLAYER1 : PLAYER2;
  }

  void advance(const Action &action) {
    int y = -1, x = -1;
    for (int i = 0; i < N; i++)
      for (int j = 0; j < N; j++) {
        if (board[i][j] == action.piece_id) {
          y = i;
          x = j;
        }
      }
    assert(y != -1);

    board[y][x] = EMPTY;
    while (true) {
      int ny = y + dy[action.direction];
      int nx = x + dx[action.direction];
      if (ny < 0 || ny >= N || nx < 0 || nx >= N)
        break;
      if (board[ny][nx] != EMPTY)
        break;
      y = ny;
      x = nx;
    }
    board[y][x] = action.piece_id;
    player_id = (player_id == PLAYER1 ? PLAYER2 : PLAYER1);
  }

  vector<Action> get_legal_actions() const {
    vector<Action> actions;
    for (int i = 0; i < N; i++)
      for (int j = 0; j < N; j++) {
        int v = board[i][j];
        if (v == EMPTY)
          continue;
        if (player_id == PLAYER1 && v < 0)
          continue;
        if (player_id == PLAYER2 && v > 0)
          continue;

        for (int d = 0; d < 4; d++) {
          int y = i + dy[d], x = j + dx[d];
          if (y < 0 || y >= N || x < 0 || x >= N)
            continue;
          if (board[y][x] != EMPTY)
            continue; // 1マス先が空ならOK
          actions.push_back(Action{v, d});
        }
      }
    return actions;
  }

  void print() const {
    for (int y = 0; y < N; y++) {
      for (int x = 0; x < N; x++)
        cout << setw(3) << board[y][x] << " ";
      cout << "\n";
    }
  }
};

struct Key {
  array<int8_t, 25> a{}; // ここに -1/0/+1 を入れる
  uint8_t turn = 0;

  bool operator==(Key const &o) const { return turn == o.turn && a == o.a; }
};

struct KeyHash {
  size_t operator()(Key const &k) const noexcept {
    size_t h = k.turn * 1469598103934665603ull;
    for (auto v : k.a) {
      // v in {-1,0,1} -> {0,1,2}
      uint8_t b = static_cast<uint8_t>(v + 1);
      h ^= b + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
    }
    return h;
  }
};

static Key make_key(const State &s) {
  Key k;
  k.turn = static_cast<uint8_t>(s.player_id);
  int t = 0;
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int v = s.board[y][x];
      if (v > 0)
        k.a[t++] = 1; // PLAYER1の駒がある
      else if (v < 0)
        k.a[t++] = -1; // PLAYER2の駒がある
      else
        k.a[t++] = 0; // 空
    }
  return k;
}

// 相手視点を自分視点へ反転
static Res flip(Res r) {
  if (r == R_WIN)
    return R_LOSE;
  if (r == R_LOSE)
    return R_WIN;
  return R_DRAW;
}

// 非再帰 solve：memo を埋めつつ init の結果を返す
static Res solve_iter(const State &init,
                      unordered_map<Key, Res, KeyHash> &memo) {
  unordered_set<Key, KeyHash> instack;

  struct Frame {
    State s;
    Key k;
    vector<Action> acts;
    size_t idx = 0;
    bool has_draw = false;
    bool expanded = false; // 子を展開したか
  };

  auto immediate = [&](const State &s, Res &out) -> bool {
    int w = s.winner();
    if (w != 0) {
      out = (w == s.player_id) ? R_WIN : R_LOSE;
      return true;
    }
    auto acts = s.get_legal_actions();
    if (acts.empty()) {
      out = R_LOSE;
      return true;
    }
    return false;
  };

  Key rootk = make_key(init);
  if (auto it = memo.find(rootk); it != memo.end())
    return it->second;

  vector<Frame> st;
  st.push_back(Frame{init, rootk, {}, 0, false, false});

  while (!st.empty()) {
    Frame &f = st.back();

    // 既知なら戻る
    if (auto it = memo.find(f.k); it != memo.end()) {
      st.pop_back();
      continue;
    }

    // 終端判定
    Res term;
    if (immediate(f.s, term)) {
      memo[f.k] = term;
      st.pop_back();
      continue;
    }

    // ここから展開
    if (!f.expanded) {
      // サイクル検出
      if (instack.find(f.k) != instack.end()) {
        memo[f.k] = R_DRAW;
        st.pop_back();
        continue;
      }
      instack.insert(f.k);
      f.acts = f.s.get_legal_actions();
      f.idx = 0;
      f.has_draw = false;
      f.expanded = true;
    }

    // 全子を処理し終えたら確定
    if (f.idx >= f.acts.size()) {
      memo[f.k] = (f.has_draw ? R_DRAW : R_LOSE);
      instack.erase(f.k);
      st.pop_back();
      continue;
    }

    // 次の子
    Action a = f.acts[f.idx];

    State ns = f.s;
    ns.advance(a);
    Key ck = make_key(ns);

    // 子が探索中 → DRAW 扱い（子視点がDRAWなので自視点もDRAW）
    if (instack.find(ck) != instack.end()) {
      f.has_draw = true;
      f.idx++;
      continue;
    }

    // 子が確定済みなら反映
    if (auto it = memo.find(ck); it != memo.end()) {
      Res my = flip(it->second);
      if (my == R_WIN) {
        memo[f.k] = R_WIN;
        instack.erase(f.k);
        st.pop_back();
        continue;
      }
      if (my == R_DRAW)
        f.has_draw = true;
      f.idx++;
      continue;
    }

    // 未確定の子を積む
    st.push_back(Frame{ns, ck, {}, 0, false, false});
  }

  return memo[rootk];
}

// WIN なら「勝ちに行く手順」の一例を復元
static vector<Action>
extract_winning_line(State s, unordered_map<Key, Res, KeyHash> &memo,
                     int max_ply = 200) {
  vector<Action> line;
  for (int ply = 0; ply < max_ply; ply++) {
    Res cur = memo[make_key(s)];
    if (cur != R_WIN)
      break;
    if (s.winner() != 0)
      break;

    auto acts = s.get_legal_actions();
    bool found = false;
    for (const auto &a : acts) {
      State ns = s;
      ns.advance(a);
      Res child = memo[make_key(ns)];
      Res my = flip(child);
      if (my == R_WIN) {
        line.push_back(a);
        s = ns;
        found = true;
        break;
      }
    }
    if (!found)
      break;
  }
  return line;
}

int main() {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  State init;

  unordered_map<Key, Res, KeyHash> memo;
  memo.reserve(1 << 20);

  Res r = solve_iter(init, memo);
  cout << "Result from initial position: " << res_name(r) << "\n";

  if (r == R_WIN) {
    auto line = extract_winning_line(init, memo, 200);
    cout << "One winning line (up to 200 plies):\n";
    State s = init;
    for (int i = 0; i < (int)line.size(); i++) {
      const auto &a = line[i];
      cout << "ply " << i << " P" << s.player_id << " piece " << a.piece_id
           << " dir " << dir_name(a.direction) << "\n";
      s.advance(a);
      if (s.winner() != 0) {
        cout << "Reached center. Winner = P" << s.winner() << "\n";
        break;
      }
    }
  } else if (r == R_DRAW) {
    cout << "Note: DRAW means repetition/cycle treated as draw.\n";
  } else {
    cout << "First player cannot force a win under these rules.\n";
  }
  return 0;
}
