#include<graphics.h>
#include<string>
#include<vector>

const int window_width = 1280;
const int window_height = 720;
const int button_width = 192;
const int button_height = 75;

int player_speed = 3;//����ƶ��ٶ�
int player_life = 3;//�������ֵ
int DifficultyLevel;//�Ѷȵȼ�
int bullet_num;//�ӵ�����

ExMessage msg;

IMAGE image_background;
IMAGE img_menu;
IMAGE img_shadow;

bool is_game_started = false;
bool running = true;
bool difficult_selected = false;

#pragma comment(lib,"MSIMG32.LIB")
#pragma comment(lib,"Winmm.lib")

inline void putimage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,
		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

//��ť
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
	enum class Status {//��ť״̬
		Idle = 0,
		Hovered,
		Pushed

	};


	RECT region;//������ťλ�����С
	IMAGE img_idle;//��ʼ״̬ͼ��
	IMAGE img_hovered;//�����ͣ״̬ͼ��
	IMAGE img_pushed;//�����״̬ͼ��
	Status status = Status::Idle;


	//��������
	bool CheckCursorHit(int x, int y) {
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;

	}

};

//��Ϸ��ʼ��ť
class StartGameButton :public Button {
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~StartGameButton() = default;

protected:
	void OnClick() {
		is_game_started = true;
		
	}
};

//�˳���Ϸ��ť
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

//�ѶȰ�ť
class EasyButton :public Button {
public:
	EasyButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~EasyButton() = default;
protected:
	void OnClick() {
		DifficultyLevel = 1;
		//���ű�������
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
	}

};

class MidButton :public Button {
public:
	MidButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~MidButton() = default;
protected:
	void OnClick() {
		DifficultyLevel = 2;
		//���ű�������
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
	}

};

class DifficultButton :public Button {
public:
	DifficultButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~DifficultButton() = default;
protected:
	void OnClick() {
		DifficultyLevel = 3;
		//���ű�������
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
	}

};

//���Ŷ�����
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
	int timer = 0;//������ʱ��
	int idx_frame = 0;//����֡����
	int interval_ms = 0;
	std::vector<IMAGE*>frame_list;
};




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

		//�������ɱ߽�
		enum class SpanEdge {
			up = 0,
			down,
			left,
			right
		};
		//�����˷����ڵ�ͼ��߽紦�����λ��
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
		//���ӵ���ЧΪ�㡣�жϵ��Ƿ��ڵ��˾�����
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
	bool alive = true;//���״̬

};

//�����µ���;
void TryGenerateEnemy(std::vector<Enemy*>& enemy_list) {
	const int INTERVAL = 100;
	static int counter = 0;
	if (++counter % INTERVAL == 0)
		enemy_list.push_back(new Enemy());

}

//�����ӵ�λ��
void UpdateBullets(std::vector<Bullet>& bullet_list, const Player& player) {
	const double RADIAL_SPEED = 0.0045;//�����ٶ�
	const double TANGENT_SPEED = 0.0055;//�����ٶ�
	double radian_interval = 2 * 3.14159 / bullet_list.size();//�ӵ�֮��Ļ��ȼ��
	POINT player_position = player.GetPosition();
	double radius = 100 + 25 * sin(GetTickCount() * RADIAL_SPEED);
	for (size_t i = 0; i < bullet_list.size(); i++) {
		double radian = GetTickCount() * TANGENT_SPEED + radian_interval * i;//�ӵ���ǰ���ڻ���ֵ
		bullet_list[i].position.x = player_position.x + player.frame_width / 2 + (int)(radius * sin(radian));
		bullet_list[i].position.y = player_position.y + player.frame_height / 2 + (int)(radius * cos(radian));

	}


}

//������ҵ÷�
void DrawPlayerScore(int score) {
	static TCHAR text[64];
	_stprintf_s(text, _T("��ǰ��ҵ÷֣�%d"), score);
	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
}
//�����������ֵ
void DrawPlayerLife(int life) {
	static TCHAR textlife[64];
	_stprintf_s(textlife, _T("��ҵ�ǰ����ֵ��%d"), life);
	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 30, textlife);
}

int main() {
	initgraph(1280, 720);

	//���ر�������
	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
	//���ػ�������
	mciSendString(_T("open mus/amagi.mp3 alias hit"), NULL, 0, NULL);
	//���ص����������ײ����
	mciSendString(_T("open mus/enemy_hit.mp3 alias enemy_hit"), NULL, 0, NULL);

	Player player;

	int score = 0;//��¼��Ϸ�÷�
	
	loadimage(&image_background, _T("img/background.png"));
	loadimage(&img_shadow, _T("img/shadow_player.png"));
	loadimage(&img_menu, _T("img/menu.png"));

	std::vector<Enemy*> enemy_list;
	
	//��ʼ��Ϸ���˳���Ϸ��ťλ����Ϣ
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

	//�Ѷ�ѡ��ťλ����Ϣ
	RECT region_btn_easy, region_btn_mid, region_btn_difficult;
	region_btn_easy.left = (window_width - button_width) / 2;
	region_btn_easy.right = region_btn_easy.left + button_width;
	region_btn_easy.top = 400;
	region_btn_easy.bottom = region_btn_easy.top + button_height;

	region_btn_mid.left = (window_width - button_width) / 2;
	region_btn_mid.right = region_btn_mid.left + button_width;
	region_btn_mid.top = 490;
	region_btn_mid.bottom = region_btn_mid.top + button_height;

	region_btn_difficult.left = (window_width - button_width) / 2;
	region_btn_difficult.right = region_btn_difficult.left + button_width;
	region_btn_difficult.top = 580;
	region_btn_difficult.bottom = region_btn_difficult.top + button_height;

	EasyButton btn_easy = EasyButton(region_btn_easy, _T("img/ui_easy_idle.png"), _T("img/ui_easy_hovered.png"), _T("img/ui_easy_pushed.png"));
	MidButton btn_mid = MidButton(region_btn_mid, _T("img/ui_mid_idle.png"), _T("img/ui_mid_hovered.png"), _T("img/ui_mid_pushed.png"));
	DifficultButton btn_difficult = DifficultButton(region_btn_difficult, _T("img/ui_difficult_idle.png"), _T("img/ui_difficult_hovered.png"), _T("img/ui_difficult_pushed.png"));

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
			if (DifficultyLevel == 0) {
				while (peekmessage(&msg)) {
					BeginBatchDraw();
					cleardevice();
					btn_easy.ProcessEvent(msg);
					btn_mid.ProcessEvent(msg);
					btn_difficult.ProcessEvent(msg);
					putimage(0, 0, &img_menu);
					btn_easy.Draw();
					btn_mid.Draw();
					btn_difficult.Draw();
					FlushBatchDraw();

				}
			}
			else {
				if (DifficultyLevel == 1) {
					bullet_num = 3;
				}
				else if (DifficultyLevel == 2) {
					bullet_num = 2;
				}
				else {
					bullet_num = 1;
				}
				DWORD start_time = GetTickCount();//��ȡ�˴�ѭ����ʼʱ��

				player.move();

				std::vector<Bullet>bullet_list(bullet_num);

				TryGenerateEnemy(enemy_list);
				UpdateBullets(bullet_list, player);

				for (Enemy* enemy : enemy_list)
					enemy->move(player);

				//�����˺���ҵ���ײ
				for (Enemy* enemy : enemy_list) {
					if (enemy->CheckPlayerCollision(player)) {
						player_life--;
						//���ŵ��˺������ײ����
						mciSendString(_T("play enemy_hit from 0"), NULL, 0, NULL);
						enemy->Hurt();
						if (player_life < 1) {
							static TCHAR text[128];
							_stprintf_s(text, _T("���յ÷֣�%d !"), score);
							MessageBox(GetHWnd(), text, _T("��Ϸ����"), MB_OK);
							running = false;
							break;
						}
					}
				}

				//�����˺��ӵ�����ײ
				for (Enemy* enemy : enemy_list) {
					for (const Bullet& bullet : bullet_list) {
						if (enemy->CheckBulletCollision(bullet)) {
							//���Ż�������
							mciSendString(_T("play hit from 0"), NULL, 0, NULL);
							enemy->Hurt();
							score++;
						}
					}

				}

				//�Ƴ������ĵ���
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
				DrawPlayerLife(player_life);

				FlushBatchDraw();

				DWORD end_time = GetTickCount();
				DWORD delta_time = end_time - start_time;
				if (delta_time < 1000 / 120) {
					Sleep(1000 / 120 - delta_time);
				}
			}
		}
	}
	EndBatchDraw();
	return 0;
}