#include <bits/stdc++.h>
using namespace std;

/*
2人用対戦ゲーム。
5×5の盤面で、自身に近い1列に駒が置いてある。特に、真ん中には王様が置いてある。
交互に駒を動かす。
動かせるのは自分の駒だけで、1手先に空きマスがある方向だけ。
動かすとスケートのように壁か駒にあたる手前まで進む。
先に王様を真ん中に置いたら勝ち。
*/

constexpr int N = 5;
constexpr int EMPTY = 0, PLAYER1 = 1, PLAYER2 = 2;

constexpr int UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3;
const int dy[] = {-1, 1, 0, 0};
const int dx[] = {0, 0, -1, 1};

struct MoveLog {
  int ply;
  int player;
  int piece;
  int dir; // 0..3
  int fy, fx;
  int ty, tx;
};

static const char *dir_name(int d) {
  static const char *s[] = {"UP", "DOWN", "LEFT", "RIGHT"};
  return s[d];
}

struct Action {
  int piece_id;
  int direction;
};

class State {
public:
  vector<vector<int>> board;
  int player_id;

  State() {
    board = vector<vector<int>>(N, vector<int>(N, EMPTY));
    for (int i = 0; i < N; i++)
      board[0][i] = -(i + 1); // PLAYER2
    for (int i = 0; i < N; i++)
      board[4][i] = (i + 1); // PLAYER1
    player_id = PLAYER1;
  }

  MoveLog advance_with_log(Action action, int ply) {
    int cur_player = player_id;

    int y = -1, x = -1;
    for (int i = 0; i < N; i++)
      for (int j = 0; j < N; j++) {
        if (board[i][j] == action.piece_id) {
          y = i;
          x = j;
        }
      }
    assert(y != -1);

    int fy = y, fx = x;
    board[y][x] = EMPTY;

    while (true) {
      int ny = y + dy[action.direction], nx = x + dx[action.direction];
      if (ny < 0 || ny >= N || nx < 0 || nx >= N)
        break;
      if (board[ny][nx] != EMPTY)
        break;
      y = ny;
      x = nx;
    }

    board[y][x] = action.piece_id;
    player_id = (player_id == PLAYER1 ? PLAYER2 : PLAYER1);

    return MoveLog{ply, cur_player, action.piece_id, action.direction, fy, fx,
                   y,   x};
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
            continue; // 1マス先が空きのみ
          actions.push_back(Action{v, d});
        }
      }
    return actions;
  }

  // 勝者がいれば PLAYER1/PLAYER2 を返す。いなければ 0。
  int winner() const {
    if (board[2][2] == 3) {
      return PLAYER1;
    } else if (board[2][2] == -3) {
      return PLAYER2;
    } else {
      return 0;
    }
  }

  void print() const {
    for (int i = 0; i < N; i++) {
      for (int j = 0; j < N; j++)
        cout << setw(3) << board[i][j] << " ";
      cout << "\n";
    }
  }
};

int main() {
  std::mt19937 rng(
      (unsigned)std::chrono::steady_clock::now().time_since_epoch().count());

  State state;
  while (true) {
    auto actions = state.get_legal_actions();
    if (actions.empty()) {
      cout << "No legal moves. Game Over.\n";
      break;
    }

    int action_id = (int)(rng() % actions.size());
    int ply = 0;
    auto log = state.advance_with_log(actions[action_id], ply++);
    cout << "{"
         << "\"ply\":" << log.ply << ",\"player\":" << log.player
         << ",\"piece\":" << log.piece << ",\"dir\":\"" << dir_name(log.dir)
         << "\""
         << ",\"from\":[" << log.fy << "," << log.fx << "]"
         << ",\"to\":[" << log.ty << "," << log.tx << "]"
         << "}\n";

    int w = state.winner();
    if (w != 0) {
      cout << "Player " << w << " wins!\n";
      state.print();
      break;
    }
  }
}
