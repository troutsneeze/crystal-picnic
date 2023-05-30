#ifndef BATTLE_TRANSITION_IN_H
#define BATTLE_TRANSITION_IN_H

namespace Wrap {
	class Bitmap;
}

void battle_transition_in(Wrap::Bitmap *start_bmp, Wrap::Bitmap *end_bmp);
void cleanup_battle_transition_in();

#endif
