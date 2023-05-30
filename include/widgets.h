#ifndef WIDGETS_H
#define WIDGETS_H

#include <list>
#include <string>
#include <vector>

#include <allegro5/allegro.h>

#include <tgui2.hpp>
#include <tgui2_widgets.hpp>

#include "equipment.h"
#include "general.h"
#include "general_types.h"

class Animation_Set;

namespace Wrap {
	class Bitmap;
}

typedef void (*W_Notifier)(void *data, float row_y_pixel);

class W_I_Scrollable {
public:
	virtual void set_value(float v) = 0;

	void setSyncedWidget(W_I_Scrollable *w) {
		synced_widget = w;
	}

	W_I_Scrollable() :
		synced_widget(NULL)
	{
	}

protected:
	W_I_Scrollable *synced_widget;
};

class W_I_Toggleable {
public:
	void setOn(bool on) {
		this->on = on;
	}

	bool getOn() {
		return on;
	}

protected:
	W_I_Toggleable() :
		on(false)
	{
	}

	bool on;
};

class CrystalPicnic_Widget : public tgui::TGUIWidget {
public:
	virtual void setOffset(General::Point<float> offset) {
		this->offset = offset;
	}

	virtual General::Point<float> getOffset() {
		return offset;
	}

	CrystalPicnic_Widget() :
		offset(General::Point<float>(0, 0))
	{
	}

	virtual ~CrystalPicnic_Widget() {}

protected:
	General::Point<float> offset;
};

class W_Button : public CrystalPicnic_Widget {
public:
	virtual void draw(int abs_x, int abs_y);
	bool acceptsFocus();
	void keyDown(int keycode);
	void joyButtonDown(int button);
	void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	TGUIWidget *update();
	void setSize(int w, int h);
	Wrap::Bitmap *getImage();
	void setImageFlags(int f);
	void set_sample_name(std::string name);
	void set_enabled(bool enabled);
	void set_text_color(ALLEGRO_COLOR text_color);
	void set_text_yoffset(int ty) { text_yoffset = ty; }
	void set_text(std::string text) { this->text = text; }

	W_Button(int x, int y, int width, int height);
	W_Button(std::string filename);
	W_Button(std::string filename, std::string text);
	virtual ~W_Button();

protected:
	std::string filename;
	bool pressed;
	Wrap::Bitmap *image;
	int flags;
	std::string sample_name;
	Wrap::Bitmap *disabled_image;
	bool enabled;
	std::string text;
	ALLEGRO_COLOR text_color;
	int text_yoffset;
};

class W_Title_Screen_Icon : public W_Button {
public:

	void losingFocus();
	W_Title_Screen_Icon(std::string filename);
	virtual ~W_Title_Screen_Icon();
};

class W_Translated_Button : public CrystalPicnic_Widget {
public:
	virtual void draw(int abs_x, int abs_y);
	bool acceptsFocus();
	void keyDown(int keycode);
	void joyButtonDown(int button);
	void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	TGUIWidget *update();

	W_Translated_Button(std::string text);
	virtual ~W_Translated_Button();

protected:
	bool pressed;
	std::string text;
	std::string sample_name;
};

class W_SaveLoad_Button : public W_Button {
public:
	void draw(int abs_x, int abs_y);

	W_SaveLoad_Button(int x, int y, int width, int height, std::vector<std::string> players, double playtime, std::string area_name);
	virtual ~W_SaveLoad_Button();

protected:
	int num_players;
	double playtime;
	std::string area_name;
	Wrap::Bitmap *player_bmps[3];
	std::vector<std::string> players;
};

class W_Title_Screen_Button : public W_Button
{
public:
	void losingFocus();
	void draw(int abs_x, int abs_y);
	W_Title_Screen_Button(std::string text);
	virtual ~W_Title_Screen_Button();
};

class W_Icon : public CrystalPicnic_Widget {
public:
	virtual void draw(int abs_x, int abs_y);

	W_Icon(Wrap::Bitmap *bitmap);

	virtual ~W_Icon() {
	}

protected:
	Wrap::Bitmap *bitmap;
};

class W_FontScale_Icon : public W_Icon {
public:
	void draw(int abs_x, int abs_y);

	W_FontScale_Icon(Wrap::Bitmap *bitmap);

	virtual ~W_FontScale_Icon() {
	}
};

class W_Integer : public CrystalPicnic_Widget {
public:
	void draw(int abs_x, int abs_y);

	int get_number()
	{
		return number;
	}

	void set_number(int n)
	{
		number = n;
	}

	W_Integer(int start) :
		number(start)
	{
		color = al_map_rgb(0x00, 0x00, 0x00);
		width = 1;
		height = 1;
	}

	virtual ~W_Integer()
	{
	}

protected:
	int number;
	ALLEGRO_COLOR color;
};

class W_Icon_Button : public W_Button {
public:
	void draw(int abs_x, int abs_y);

	W_Icon_Button(std::string caption, std::string bg_name, std::string icon_name);

	virtual ~W_Icon_Button();

protected:
	std::string caption;
	std::string icon_filename;

	Wrap::Bitmap *icon;
		
	int icon_w;
	int icon_h;
};

class W_Scrolling_List : public CrystalPicnic_Widget, public W_I_Scrollable {
public:
	static const int LEFT = 1;
	static const int RIGHT= 2;
	static const int UP = 3;
	static const int DOWN = 4;
	static const int A = 5;
	static const int B = 6;

	bool acceptsFocus();

	virtual void use_button(int button);
	virtual bool keyChar(int keycode, int unichar);
	virtual void joyButtonDown(int button);
	virtual bool joyAxisRepeat(int stick, int axis, float value);
	int get_num_items();
	void set_translate_item_names(bool translate);
	virtual void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	virtual void mouseUp(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	virtual void mouseMove(int rel_x, int rel_y, int abs_x, int abs_y);
	void show_selected();
	virtual void draw(int abs_x, int abs_y);
	virtual TGUIWidget *update();
	void set_value(float v);
	int get_selected();
	void set_selected(int sel);
	void setHeight(int h);
	void setNotifier(W_Notifier n, void *data);
	// returns percent of max height
	float get_scrollbar_tab_size();
	void set_tint_icons(bool tint);
	virtual void remove_item(int index);
	void move(int index, int dir);
	void move_up();
	void move_down();
	void update_item(int index, bool update_name, std::string name, bool update_icon, std::string icon_filename, bool update_right_justified_text, std::string _right_justified_text);
	std::string get_item_name(int index);
	void set_right_icon_filenames(std::vector<std::string> right_icon_filenames);
	bool get_right_icon_clicked();
	bool is_activated();
	void set_can_arrange(bool can_arrange);
	void set_item_images(std::vector<Wrap::Bitmap *> *images);
	W_Scrolling_List(std::vector<std::string> item_names, std::vector<std::string> icon_filenames, std::vector<std::string> right_justified_text, std::vector<bool> disabled, General::Font_Type font, bool draw_box);
	virtual ~W_Scrolling_List();

protected:
	static const int POINTS_TO_TRACK = 10;
	static const float DECELLERATION;
	static const int MAX_VELOCITY = 500;

	void set_velocity() {
		if (move_points.size() > 1) {
			std::pair<double, General::Point<int> > p_start = move_points[move_points.size()-1];
			std::pair<double, General::Point<int> > p_end = move_points[0];
			double elapsed = p_end.first - p_start.first;
			float travelled = p_end.second.y - p_start.second.y;
			vy = -travelled / elapsed;
			if (vy > MAX_VELOCITY)
				vy = MAX_VELOCITY;
			else if (vy < -MAX_VELOCITY)
				vy = -MAX_VELOCITY;
		}
	}

	int selected;
	std::vector<std::string> item_names;
	General::Font_Type font;
	
	std::vector<std::string> icon_filenames;
	std::vector<Wrap::Bitmap *> icons;
	std::vector<std::string> right_justified_text;
	std::vector<bool> disabled;

	float y_offset;
	float max_y_offset;
	float vy; // pixels/second

	bool mouse_is_down;
	General::Point<int> real_mouse_down_point;
	General::Point<int> mouse_down_point;
	std::vector< std::pair<double, General::Point<int> > > move_points;

	bool can_notify;
	W_Notifier notifier;
	void *notifier_data;

	bool draw_box;
	bool tint_icons;

	std::vector<std::string> right_icon_filenames;
	std::vector<Wrap::Bitmap *> right_icons;

	bool right_icon_clicked;

	bool activated;
	int active_column;

	bool translate_item_names;

	bool can_arrange;

	std::vector<Wrap::Bitmap *> *item_images;
};

class W_Equipment_List : public W_Scrolling_List {
public:
	void use_button(int button);
	bool keyChar(int keycode, int unichar);
	void joyButtonDown(int button);
	bool joyAxisRepeat(int stick, int axis, float value);
	void update_description(int index, std::string desc);
	void set_active_column(int c);
	void deactivate();
	void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	void mouseUp(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	void mouseMove(int rel_x, int rel_y, int abs_x, int abs_y);
	void draw(int abs_x, int abs_y);
	void postDraw(int abs_x, int abs_y);
	void insert_item(int pos, std::string icon_filename, std::string text, std::string right_icon_filename, Equipment::Equipment_Type equip_type, std::string desc, bool disabled);
	void remove_item(int index);
	Equipment::Equipment_Type get_type(int index);
	std::string get_item_name(int index);
	void set_draw_outline(bool draw_outline);

	W_Equipment_List(std::vector<std::string> item_names, std::vector<std::string> icon_filenames, std::vector<Equipment::Equipment_Type> equipment_type, std::vector<std::string> descriptions, std::vector<bool> disabled, General::Font_Type font, bool draw_box);
	virtual ~W_Equipment_List();

protected:

	std::vector<Equipment::Equipment_Type> equipment_type;
	std::vector<std::string> descriptions;
	Wrap::Bitmap *info_bmp;

	bool draw_info_box;
	float info_box_x;
	float info_box_y;
	std::string info_box_text;
	bool draw_outline;
};

class W_Vertical_Scrollbar : public CrystalPicnic_Widget, public W_I_Scrollable {
public:
	static const int WIDTH = 5;

	void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);

	void mouseUp(int rel_x, int rel_y, int abs_x, int abs_y, int mb) {
		if (down) {
			down = false;
		}
	}

	void mouseMove(int rel_x, int rel_y, int abs_x, int abs_y);

	void draw(int abs_x, int abs_y);

	TGUIWidget *update() {
		return NULL;
	}

	float get_value() {
		return value;
	}

	void set_value(float v) {
		value = v;
	}

	void setHeight(int h);

	int get_tab_size();

	void set_tab_size(float tab_size)
	{
		f_tab_size = tab_size;
		setHeight(height);
		if (synced_widget) {
			synced_widget->set_value(value);
		}
	}
	
	#define TOP_NAME "misc_graphics/interface/vertical_scrollbar_top.png"
	#define MID_NAME "misc_graphics/interface/vertical_scrollbar_middle.png"

	W_Vertical_Scrollbar(float tab_size);

	virtual ~W_Vertical_Scrollbar();
	
	#undef TOP_NAME
	#undef MID_NAME

protected:
	void set(int trough_pos);

	bool down;
	General::Point<int> down_point;
	int travel;
	float value;

	Animation_Set *trough_anim;
	Animation_Set *tab_anim;

	float f_tab_size;
};

class Button_Group {
public:
	Button_Group(std::list<W_I_Toggleable *> group) :
		group(group)
	{
	}

	void toggle(W_I_Toggleable *widget) {
		std::list<W_I_Toggleable *>::iterator it;
		for (it = group.begin(); it != group.end(); it++) {
			W_I_Toggleable *w = *it;
			if (w == widget)
				w->setOn(true);
			else
				w->setOn(false);
		}
	}

protected:
	std::list<W_I_Toggleable *> group;
};

class W_Checkbox : public TGUI_Checkbox {
public:
	void keyDown(int keycode);
	void joyButtonDown(int keycode);
	bool acceptsFocus();
	void draw(int abs_x, int abs_y);

	W_Checkbox(int x, int y, bool checked, std::string text);

private:
	std::string text;
};

class W_Slider : public TGUI_Slider {
public:
	bool keyChar(int keycode, int unichar);
	bool joyAxisRepeat(int stick, int axis, float value);
	bool acceptsFocus();
	void draw(int abs_x, int abs_y);

	W_Slider(int x, int y, int size);
};

class W_Audio_Settings_Button : public W_Button {
public:
	void keyDown(int keycode);
	void joyButtonDown(int button);
	void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);

	void set_widgets(W_Slider *sfx_slider, W_Slider *music_slider, W_Checkbox *reverb_checkbox) {
		this->sfx_slider = sfx_slider;
		this->music_slider = music_slider;
		this->reverb_checkbox = reverb_checkbox;
	};

	W_Audio_Settings_Button(std::string filename, std::string text);

private:
	void apply();

	W_Slider *sfx_slider;
	W_Slider *music_slider;
	W_Checkbox *reverb_checkbox;
};

#endif // WIDGETS_H
