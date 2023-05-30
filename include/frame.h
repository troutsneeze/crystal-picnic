#ifndef FRAME_HPP
#define FRAME_HPP

class Bitmap;

class Frame {
public:
	Bitmap *get_bitmap();
	int get_delay();

	int get_width();
	int get_height();

	Frame(Bitmap *bitmap, int delay);
	~Frame();

protected:
	Bitmap *bitmap;
	int delay;
};

#endif

