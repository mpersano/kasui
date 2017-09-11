#pragma once

struct block_info {
	wchar_t kanji;
	struct texuv {
		float u0, v0, u1, v1;
	} texuvs[2];
};
