#pragma once

#include <vector>
#include <string>
#include <memory>

#include "action.h"
#include "world.h"

struct jukugo;
struct gradient;

namespace g2d {
class font;
class texture;
};

class jukugo_info_sprite : public sprite
{
public:
	jukugo_info_sprite(const jukugo *jukugo_info, float x, float y, const gradient *g);

	bool update(uint32_t dt) override;
	void draw() const override;

private:
	const jukugo *jukugo_;

	float x_base_, y_base_;

	float flip_ = 0;
	float alpha_ = 0;
	float z_ = 0;
	std::unique_ptr<abstract_action> action_;

	const gradient *gradient_;

	const g2d::font *kanji_font_;
	const g2d::font *furigana_font_;
	const g2d::font *eigo_font_;

	g2d::vec2 pos_kanji_;
	g2d::vec2 pos_furigana_;
	std::vector<std::pair<g2d::vec2, std::wstring>> eigo_lines_;
};
