#include<graphics.h>
#include<string>
#include<vector>

const int window_width = 1280;
const int window_height = 720;
const int button_width = 192;
const int button_height = 75;

int player_speed = 3;

ExMessage msg;

#pragma comment(lib,"MSIMG32.LIB")
#pragma comment(lib,"Winmm.lib")

bool is_game_started = false;
bool running = true;

inline void putimage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,
		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}


//按钮
class Button {
public:
	Button(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed) {
		region = rect;
		loadimage(&img_idle, path_img_idle);
		loadimage(&img_hovered, path_img_hovered);
		loadimage(&img_pushed, path_img_pushed);
	}

	~Button() = default;

	void ProcessEvent(const ExMessage& msg) {
		switch (msg.message) {
		case WM_MOUSEMOVE:
			if (status == Status::Idle && CheckCursorHit(msg.x, msg.y)) {
				status = Status::Hovered;
			}
			else if (status == Status::Hovered && !CheckCursorHit(msg.x, msg.y)) {
				status = Status::Idle;
			}
			break;
		case WM_LBUTTONDOWN:
			if (CheckCursorHit(msg.x, msg.y)) {
				status = Status::Pushed;
			}
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed) {
				OnClick();
			}
			break;
		defult:
			break;
		}
	}




	void Draw() {
		switch (status) {
		case Status::Idle:
			putimage(region.left, region.top, &img_idle);
			break;
		case Status::Hovered:
			putimage(region.left, region.top, &img_hovered);
			break;
		case Status::Pushed:
			putimage(region.left, region.top, &img_pushed);
			break;
		}
	}


protected:
	virtual void OnClick() = 0;


private:
	enum class Status {//按钮状态
		Idle = 0,
		Hovered,
		Pushed

	};


	RECT region;//描述按钮位置与大小
	IMAGE img_idle;//初始状态图像
	IMAGE img_hovered;//鼠标悬停状态图像
	IMAGE img_pushed;//鼠标点击状态图像
	Status status = Status::Idle;


	//检测鼠标点击
	bool CheckCursorHit(int x, int y) {
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;

	}

};

//游戏开始按钮
class StartGameButton :public Button {
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~StartGameButton() = default;

protected:
	void OnClick() {
		is_game_started = true;
		//播放背景音乐
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
	}
};

//退出游戏按钮
class QuitGameButton :public Button {
public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~QuitGameButton() = default;

protected:
	void OnClick() {
		running = false;
	}
};



class Animation {
public:
	Animation(LPCTSTR path, int num, int interval) {
		interval_ms = interval;

		TCHAR path_file[256];
		for (size_t i = 0; i < num; i++) {
			_stprintf_s(path_file, path, i);
			IMAGE* frame = new IMAGE();
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}
	}


	~Animation() {
		for (size_t i = 0; i < frame_list.size(); i++) {
			delete frame_list[i];
		}
	}


	void play(int x, int y, int delta) {
		timer += delta;
		if (timer >= interval_ms) {
			idx_frame = (idx_frame + 1) % frame_list.size();
			timer = 0;
		}

		putimage_alpha(x, y, frame_list[idx_frame]);
	}


private:
	int timer = 0;//动画计时器
	int idx_frame = 0;//动画帧索引
	int interval_ms = 0;
	std::vector<IMAGE*>frame_list;
};


IMAGE img_shadow;

class Player {
public:
	bool is_move_up = false;
	bool is_move_down = false;
	bool is_move_left = false;
	bool is_move_right = false;

	void move() {
		while (peekmessage(&msg)) {
			if (msg.message == WM_KEYDOWN) {
				switch (msg.vkcode) {
				case VK_UP:
					is_move_up = true;
					break;
				case VK_DOWN:
					is_move_down = true;
					break;
				case VK_LEFT:
					is_move_left = true;
					break;
				case VK_RIGHT:
					is_move_right = true;
					break;

				}
			}
			else if (msg.message == WM_KEYUP) {
				switch (msg.vkcode) {
				case VK_UP:
					is_move_up = false;
					break;
				case VK_DOWN:
					is_move_down = false;
					break;
				case VK_LEFT:
					is_move_left = false;
					break;
				case VK_RIGHT:
					is_move_right = false;
					break;

				}
			}

		}

		int dir_x = is_move_right - is_move_left;
		int dir_y = is_move_down - is_move_up;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (len_dir != 0) {
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			player_pos.x += (int)(player_speed * normalized_x);
			player_pos.y += (int)(player_speed * normalized_y);
		}

		if (player_pos.x < 0)player_pos.x = 0;
		if (player_pos.y < 0)player_pos.y = 0;
		if (player_pos.x + frame_width > window_width)player_pos.x = window_width - frame_width;
		if (player_pos.y + frame_height > window_height)player_pos.y = window_height - frame_height;
	}

	void Draw(int delta) {
		int dir_x = is_move_right - is_move_left;
		int pos_shadow_x = player_pos.x + (frame_width / 2 - shadow_width / 2);
		int pos_shadow_y = player_pos.y + frame_height - 8;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);

		static bool facing_left = false;

		if (dir_x < 0) {
			facing_left = true;
		}
		else if (dir_x > 0) {
			facing_left = false;
		}
		if (facing_left) {
			anim_left->play(player_pos.x, player_pos.y, delta);
		}
		else anim_right->play(player_pos.x, player_pos.y, delta);


	}

	const POINT& GetPosition()const {
		return player_pos;
	}


	const int frame_width = 80;
	const int frame_height = 80;
	const int shadow_width = 32;
	POINT player_pos = { 500,500 };
	Animation* anim_left = new Animation(_T("img/player_left_%d.png"), 3, 45);
	Animation* anim_right = new Animation(_T("img/player_right_%d.png"), 3, 45);

};


class Bullet {
public:
	POINT position = { 0,0 };
	Bullet() = default;
	~Bullet() = default;
	void Draw()const {
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(position.x, position.y, RADIUS);
	}

private:
	const int RADIUS = 10;
};


class Enemy {
public:
	Enemy() {
		loadimage(&img_shadow, _T("img/shadow_enemy.png"));
		anim_left = new Animation(_T("img/enemy_left_%d.png"), 6, 45);
		anim_right = new Animation(_T("img/enemy_right_%d.png"), 6, 45);

		//敌人生成边界
		enum class SpanEdge {
			up = 0,
			down,
			left,
			right
		};
		//将敌人放置在地图外边界处的随机位置
		SpanEdge edge = (SpanEdge)(rand() % 4);
		switch (edge) {
		case SpanEdge::up:
			position.x = rand() % window_width;
			position.y = -frame_height;
			break;
		case SpanEdge::down:
			position.x = rand() % window_width;
			position.y = window_height;
			break;
		case SpanEdge::left:
			position.x = -frame_width;
			position.y = rand() % window_height;
			break;
		case SpanEdge::right:
			position.x = window_width;
			position.y = rand() % window_height;
			break;
		}
	}

	bool  CheckBulletCollision(const Bullet& bullet) {
		//将子弹等效为点。判断点是否在敌人矩形内
		bool is_overlap_x = bullet.position.x >= position.x && bullet.position.x <= position.x + frame_width;
		bool is_overlap_y = bullet.position.y >= position.y && bullet.position.y <= position.y + frame_height;
		return is_overlap_x && is_overlap_y;
	}

	bool CheckPlayerCollision(const Player& player) {
		POINT check_position = { position.x + frame_width / 2,position.y + frame_height / 2 };
		POINT playerpos = player.GetPosition();
		if (check_position.x >= playerpos.x && check_position.x <= playerpos.x + player.frame_width && check_position.y >= playerpos.y && check_position.y <= playerpos.y + player.frame_height) {
			return true;
		}
		return false;
	}

	void move(const Player& play) {
		const POINT& player_position = play.GetPosition();
		int dir_x = player_position.x - position.x;
		int dir_y = player_position.y - position.y;
		if (dir_x < 0) {
			facing_left = true;
		}
		else if (dir_x > 0) {
			facing_left = false;
		}
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (len_dir != 0) {
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			position.x += (int)(speed * normalized_x);
			position.y += (int)(speed * normalized_y);
		}


	}

	void Draw(int delta) {
		int pos_shadow_x = position.x + (frame_width / 2 - shadow_width / 2);
		int pos_shadow_y = position.y + frame_height - 35;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);

		if (facing_left) {
			anim_left->play(position.x, position.y, delta);
		}
		else anim_right->play(position.x, position.y, delta);


	}

	void Hurt() {


		alive = false;
	}

	bool CheckAlive() {

		return alive;
	}

	~Enemy() {
		delete anim_left;
		delete anim_right;

	}


private:
	const int speed = 2;
	const int frame_width = 80;
	const int frame_height = 80;
	const int shadow_width = 40;

	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT position = { 0,0 };
	bool facing_left = false;
	bool alive = true;//存活状态

};

//生成新敌人;
void TryGenerateEnemy(std::vector<Enemy*>& enemy_list) {
	const int INTERVAL = 100;
	static int counter = 0;
	if (++counter % INTERVAL == 0)
		enemy_list.push_back(new Enemy());

}

//更新子弹位置
void UpdateBullets(std::vector<Bullet>& bullet_list, const Player& player) {
	const double RADIAL_SPEED = 0.0045;//径向速度
	const double TANGENT_SPEED = 0.0055;//切向速度
	double radian_interval = 2 * 3.14159 / bullet_list.size();//子弹之间的弧度间隔
	POINT player_position = player.GetPosition();
	double radius = 100 + 25 * sin(GetTickCount() * RADIAL_SPEED);
	for (size_t i = 0; i < bullet_list.size(); i++) {
		double radian = GetTickCount() * TANGENT_SPEED + radian_interval * i;//子弹当前所在弧度值
		bullet_list[i].position.x = player_position.x + player.frame_width / 2 + (int)(radius * sin(radian));
		bullet_list[i].position.y = player_position.y + player.frame_height / 2 + (int)(radius * cos(radian));

	}


}

//绘制玩家得分
void DrawPlayerScore(int score) {
	static TCHAR text[64];
	_stprintf_s(text, _T("当前玩家得分：%d"), score);
	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);


}

int main() {
	initgraph(1280, 720);

	//加载背景音乐
	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
	//加载击打音乐
	mciSendString(_T("open mus/amagi.mp3 alias hit"), NULL, 0, NULL);

	Player player;


	//Player player;
	// std::vector<Enemy*>enemy_list; 

	int score = 0;//记录游戏得分
	IMAGE image_background;
	IMAGE img_menu;
	loadimage(&image_background, _T("img/background.png"));
	loadimage(&img_shadow, _T("img/shadow_player.png"));

	std::vector<Enemy*> enemy_list;
	std::vector<Bullet>bullet_list(3);

	//开始游戏和退出游戏按钮位置信息
	RECT region_btn_start_game, region_btn_quit_game;
	region_btn_start_game.left = (window_width - button_width) / 2;
	region_btn_start_game.right = region_btn_start_game.left + button_width;
	region_btn_start_game.top = 430;
	region_btn_start_game.bottom = region_btn_start_game.top + button_height;

	region_btn_quit_game.left = (window_width - button_width) / 2;
	region_btn_quit_game.right = region_btn_quit_game.left + button_width;
	region_btn_quit_game.top = 550;
	region_btn_quit_game.bottom = region_btn_quit_game.top + button_height;

	StartGameButton btn_start_game = StartGameButton(region_btn_start_game,
		_T("img/ui_start_idle.png"), _T("img/ui_start_hovered.png"), _T("img/ui_start_pushed.png"));
	QuitGameButton btn_quit_game = QuitGameButton(region_btn_quit_game,
		_T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));

	loadimage(&img_menu, _T("img/menu.png"));

	BeginBatchDraw();
	
	while (running) {
		if (!is_game_started) {
			while (peekmessage(&msg)) {
				BeginBatchDraw();
			
				cleardevice();
				btn_start_game.ProcessEvent(msg);
				btn_quit_game.ProcessEvent(msg);
				putimage(0, 0, &img_menu);
				btn_start_game.Draw();
				btn_quit_game.Draw();
				FlushBatchDraw();
			}
		}
		else {
			DWORD start_time = GetTickCount();


			player.move();

			TryGenerateEnemy(enemy_list);
			UpdateBullets(bullet_list, player);


			for (Enemy* enemy : enemy_list)
				enemy->move(player);

			//检测敌人和玩家的碰撞
			for (Enemy* enemy : enemy_list) {
				if (enemy->CheckPlayerCollision(player)) {
					static TCHAR text[128];
					_stprintf_s(text, _T("最终得分：%d !"), score);
					MessageBox(GetHWnd(), text, _T("游戏结束"), MB_OK);
					running = false;
					break;
				}
			}

			//检测敌人和子弹的碰撞
			for (Enemy* enemy : enemy_list) {
				for (const Bullet& bullet : bullet_list) {
					if (enemy->CheckBulletCollision(bullet)) {
						//播放击打音乐
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);
						enemy->Hurt();
						score++;
					}
				}

			}

			//移除生命值归零的敌人
			for (size_t i = 0; i < enemy_list.size(); i++) {
				Enemy* enemy = enemy_list[i];
				if (!enemy->CheckAlive()) {
					std::swap(enemy_list[i], enemy_list.back());
					enemy_list.pop_back();
					delete enemy;
				}
			}

			cleardevice();
			putimage(0, 0, &image_background);
			player.Draw(1000 / 500);
			for (Enemy* enemy : enemy_list)
				enemy->Draw(1000 / 144);
			for (const Bullet& bullet : bullet_list)
				bullet.Draw();
			DrawPlayerScore(score);

			FlushBatchDraw();

			DWORD end_time = GetTickCount();
			DWORD delta_time = end_time - start_time;
			if (delta_time < 1000 / 120) {
				Sleep(1000 / 120 - delta_time);
			}


		}

	}

	EndBatchDraw();
	return 0;
}