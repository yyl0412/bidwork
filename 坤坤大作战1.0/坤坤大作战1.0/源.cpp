#include<graphics.h>
#include<string>
#include<vector>

//��Ϸ���ڴ�С
const int window_width = 1280;
const int window_height = 720;

//����ƶ��ٶ�
int player_speed = 3;

ExMessage msg;

#pragma comment(lib,"MSIMG32.LIB")
#pragma comment(lib,"Winmm.lib")


//͸����ʾͼƬ����
inline void putimage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,
		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

//���嶯��������
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

Animation anim_right(_T("img/player_right_%d.png"), 3, 45);
Animation anim_left(_T("img/player_left_%d.png"), 3, 45);
Animation anim_enemy_right(_T("img/enemy_right_%d.png"), 6, 45);
Animation anim_enemy_left(_T("img/enemy_left_%d.png"), 6, 45);

IMAGE img_shadow;

//�����
class Player {
public:
	bool is_move_up = false;
	bool is_move_down = false;
	bool is_move_left = false;
	bool is_move_right = false;
	Player() {

	}

	~Player() {

	}

//����ƶ�
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

		//x�����y������ٶ�����
		int dir_x = is_move_right - is_move_left;
		int dir_y = is_move_down - is_move_up;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (len_dir != 0) {
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			player_pos.x += (int)(player_speed * normalized_x);
			player_pos.y += (int)(player_speed * normalized_y);
		}

		//��ֹ����ƶ������߽�
		if (player_pos.x < 0)player_pos.x = 0;
		if (player_pos.y < 0)player_pos.y = 0;
		if (player_pos.x + frame_width > window_width)player_pos.x = window_width - frame_width;
		if (player_pos.y + frame_height > window_height)player_pos.y = window_height - frame_height;
	}

	void Draw(int delta) {
		int dir_x = is_move_right - is_move_left;
		//������Ӱ
		int pos_shadow_x = player_pos.x + (frame_width / 2 - shadow_width / 2);
		int pos_shadow_y = player_pos.y + frame_height - 8;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);

		//�ж������ƶ�����
		static bool facing_left = false;

		if (dir_x < 0) {
			facing_left = true;
		}
		else if (dir_x > 0) {
			facing_left = false;
		}
		if (facing_left) {
			anim_left.play(player_pos.x, player_pos.y, delta);
		}
		else anim_right.play(player_pos.x, player_pos.y, delta);

	}

	//��ȡ���λ����Ϣ
	const POINT& GetPosition()const {
		return player_pos;
	}

	//��Ҵ�С����Ӱ��С
	const int frame_width = 80;
	const int frame_height = 80;
	const int shadow_width = 32;
	POINT player_pos = { 500,500 };

};

//�ӵ���
class Bullet {
public:
	POINT position = { 0,0 };
	Bullet() = default;
	~Bullet() = default;
	//�����ӵ���״��ɫ
	void Draw()const {
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(position.x, position.y, RADIUS);
	}

private:
	const int RADIUS = 10;//�ӵ��뾶
};

//������
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
	//����ӵ�������Ƿ�����ײ
	bool  CheckBulletCollision(const Bullet& bullet) {
		//���ӵ���ЧΪ�㡣�жϵ��Ƿ��ڵ��˾�����
		bool is_overlap_x = bullet.position.x >= position.x && bullet.position.x <= position.x + frame_width;
		bool is_overlap_y = bullet.position.y >= position.y && bullet.position.y <= position.y + frame_height;
		return is_overlap_x && is_overlap_y;
	}

	//�ж����������Ƿ�����ײ
	bool CheckPlayerCollision(const Player& player) {
		POINT check_position = { position.x + frame_width / 2,position.y + frame_height / 2 };
		POINT playerpos = player.GetPosition();
		if (check_position.x >= playerpos.x && check_position.x <= playerpos.x + player.frame_width && check_position.y >= playerpos.y && check_position.y <= playerpos.y + player.frame_height) {
			return true;
		}
		return false;
	}

	//��������ƶ��߼�
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

	//�жϵ����Ƿ���
	bool CheckAlive() {
		return alive;
	}

	~Enemy() {
		delete anim_left;
		delete anim_right;

	}

private:
	const int speed = 2;//�����ƶ��ٶ�
	const int frame_width = 80;
	const int frame_height = 80;
	const int shadow_width = 40;

	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT position = { 0,0 };
	bool facing_left = false;//�����ƶ�����
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

int main() {
	initgraph(1280, 720);

	//���ر�������
	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
	//���ػ�������
	mciSendString(_T("open mus/amagi.mp3 alias hit"), NULL, 0, NULL);
	//���ű�������
	mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
	Player player;

	bool running = true;//��������״̬

	int score = 0;//��¼��Ϸ�÷�
	IMAGE image_background;//���ر���ͼ
	loadimage(&image_background, _T("img/background.png"));
	loadimage(&img_shadow, _T("img/shadow_player.png"));

	std::vector<Enemy*> enemy_list;//���ɵ�������
	std::vector<Bullet>bullet_list(3);//�����ӵ�����

	BeginBatchDraw();
	

	while (running) {
		DWORD start_time = GetTickCount();//��ȡѭ����ʼʱ��

		player.move();//����ƶ�

		TryGenerateEnemy(enemy_list);//���ɵ���
		UpdateBullets(bullet_list, player);//�����ӵ�λ��


		for (Enemy* enemy : enemy_list)
			enemy->move(player);//�����ƶ�

		//�����˺���ҵ���ײ
		for (Enemy* enemy : enemy_list) {
			if (enemy->CheckPlayerCollision(player)) {

				static TCHAR text[128];
				_stprintf_s(text, _T("���յ÷֣�%d !"), score);
				MessageBox(GetHWnd(), text, _T("��Ϸ����"), MB_OK);
				running = false;
				break;
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

		//�Ƴ�����ֵ����ĵ���
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

		DWORD end_time = GetTickCount();//��ȡѭ������ʱ��
		DWORD delta_time = end_time - start_time;//ÿ��ѭ������ʱ��
		if (delta_time < 1000 / 120) {
			Sleep(1000 / 120 - delta_time);
		}
	}
	EndBatchDraw();
	return 0;
}