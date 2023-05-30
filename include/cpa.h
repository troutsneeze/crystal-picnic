#ifndef CPA_H
#define CPA_H

#include <map>
#include <string>

#include <allegro5/allegro.h>

class CPA
{
public:
	ALLEGRO_FILE *load(std::string filename);
	bool exists(std::string filename);

	CPA(std::string archive_name);
	~CPA();

private:
	uint8_t *bytes;
	std::map< std::string, std::pair<int, int> > info; // offset, size
};

#endif
