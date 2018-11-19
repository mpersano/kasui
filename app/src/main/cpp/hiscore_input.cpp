#include "hiscore_input.h"

#include "common.h"
#include "leaderboard_page.h"
#include "main_menu.h"
#include "options.h"
#include "fonts.h"
#include "programs.h"
#include "render.h"

#include <guava2d/font.h>
#include <guava2d/program.h>
#include <guava2d/texture_manager.h>
#include <guava2d/xwchar.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>

extern void draw_message(const g2d::mat4 &, float alpha, const wchar_t *);
extern int total_tics;

namespace {
constexpr const auto KEYBOARD_LAYER = 99;
constexpr const auto INPUT_BUFFER_LAYER = 99;

enum
{
    KEY_WIDTH = 60,
    KEY_HEIGHT = 80,
    KEY_COLS = 10,

    NUM_KEYBOARD_LAYOUTS = 1,
    MAX_KEYS_PER_LAYOUT = 200,
};

enum
{
    NIL_KEY = 1,
    ABC_KEY = 2,
    HIRAGANA_KEY = 3,
    SHIFT_KEY = 4,
    MARU_TENTEN_KEY = 5,
    KATAKANA_KEY = 6,
    SMALL_KEY = 7,
};

struct key_info
{
    int width;
    float u, v;
    float du, dv;
};

std::map<wchar_t, key_info *> char_to_key;

class score_text
{
public:
    score_text(const g2d::font *font);

    void initialize(int value);

    void draw(float alpha) const;

private:
    void add_digit(wchar_t ch);
    void format_thousands(int n);

    const g2d::font *font_;
    wchar_t score_text_[30];
    size_t score_text_size_ = 0;
};

score_text::score_text(const g2d::font *font)
    : font_(font)
{
}

void score_text::add_digit(wchar_t ch)
{
    score_text_[score_text_size_++] = ch;
}

void score_text::format_thousands(int n)
{
    if (n >= 1000) {
        format_thousands(n / 1000);

        const int m = n % 1000;

        add_digit(',');
        add_digit('0' + (m / 100));
        add_digit('0' + (m / 10) % 10);
        add_digit('0' + m % 10);
    } else {
        if (n >= 100)
            add_digit('0' + (n / 100));
        if (n >= 10)
            add_digit('0' + (n / 10) % 10);
        add_digit('0' + n % 10);
    }
}

void score_text::initialize(int score)
{
    score_text_size_ = 0;
    format_thousands(score);
    score_text_[score_text_size_] = L'\0';
}

void score_text::draw(float alpha) const
{
    const auto top_color = g2d::rgb{200, 200, 255} * (1. / 255.);
    const auto bottom_color = g2d::rgb{120, 120, 255} * (1. / 255.);

    const g2d::rgba top_color_text{top_color, alpha};
    const g2d::rgba bottom_color_text{bottom_color, alpha};
    const g2d::rgba top_color_outline{.5 * top_color, alpha};
    const g2d::rgba bottom_color_outline{.5 * bottom_color, alpha};

    render::set_text_align(text_align::CENTER);
    render::draw_text(font_, {}, INPUT_BUFFER_LAYER,
                      top_color_outline, top_color_text,
                      bottom_color_outline, bottom_color_text,
                      score_text_);
}

class input_buffer
{
public:
    input_buffer();

    void reset(const wchar_t *initial_value);
    void draw() const;
    void append_char(wchar_t ch);
    void del();
    void maru_tenten();
    void small();

    const wchar_t *get_input() const { return input; }

    int get_input_len() const { return input_len; }

protected:
    enum
    {
        MAX_INPUT_LEN = 10
    };

    wchar_t input[MAX_INPUT_LEN + 1];
    int input_len;
};

input_buffer::input_buffer()
    : input_len(0)
{
}

void input_buffer::draw() const
{
    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    const auto *small_font = get_font(font::small);

    enum
    {
        MAX_CHAR_WIDTH = 44,
        LINE_HEIGHT = 64,
        LINE_WIDTH = MAX_INPUT_LEN * MAX_CHAR_WIDTH
    };

    const float y = .6 * window_height;

    float x = .5 * (window_width - LINE_WIDTH);

    auto g = small_font->find_glyph(L'X');
    const float y_text = y + .5 * g->height - g->top;

    // text

    auto *prog = get_program(program::text_inner);

    for (const wchar_t *p = input; p != &input[input_len]; p++) {
        auto g = small_font->find_glyph(*p);

        const float x0 = x + g->left;
        const float x1 = x0 + g->width;

        const float y0 = y_text + g->top;
        const float y1 = y0 - g->height;

        const g2d::vec2 &t0 = g->texuv[0];
        const g2d::vec2 &t1 = g->texuv[1];
        const g2d::vec2 &t2 = g->texuv[2];
        const g2d::vec2 &t3 = g->texuv[3];

        const auto draw_char = [=](int layer) {
            render::draw_quad(
                    prog,
                    small_font->get_texture(),
                    {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}},
                    {t0, t1, t2, t3},
                    layer);

        };

        render::set_color({1.f, 1.f, 1.f, 1.f});
        draw_char(INPUT_BUFFER_LAYER);

        render::push_matrix();
        render::translate(5, -5);
        render::set_color({0.f, 0.f, 0.f, 0.25f});
        draw_char(INPUT_BUFFER_LAYER - 1);
        render::pop_matrix();

        x += g->advance_x;
    }

    // cursor

    if ((total_tics / 512) & 1) {
        render::set_color({1.0, 1.0, 1.0, .25});

        const float x0 = x + 8;
        const float x1 = x0 + 12;
        const float y0 = y - .5 * LINE_HEIGHT + 4;
        const float y1 = y + .5 * LINE_HEIGHT - 4;

        render::draw_quad({{x0, y0}, {x0, y1}, {x1, y1}, {x1, y0}}, INPUT_BUFFER_LAYER);
    }

    // box

    {
        render::set_color({0.0, 0.0, 0.0, .25});

        const float x0 = .5 * (window_width - LINE_WIDTH);
        const float x1 = .5 * (window_width + LINE_WIDTH);

        const float y0 = y - .5 * LINE_HEIGHT;
        const float y1 = y + .5 * LINE_HEIGHT;

        render::draw_quad({{x0, y0}, {x0, y1}, {x1, y1}, {x1, y0}}, INPUT_BUFFER_LAYER - 2);
    }
}

void input_buffer::append_char(wchar_t ch)
{
    if (input_len < MAX_INPUT_LEN) {
        input[input_len++] = ch;
        input[input_len] = '\0';
    }
}

void input_buffer::del()
{
    if (input_len > 0)
        input[--input_len] = '\0';
}

void input_buffer::maru_tenten()
{
    if (input_len > 0) {
        wchar_t ch = input[input_len - 1];

        bool is_katakana;

        if (ch >= 12448 && ch <= 12543) {
            // katakana
            is_katakana = true;
            ch += L'あ' - L'ア';
        } else {
            is_katakana = false;
        }

        switch (ch) {
            // かきくけこ

            case L'か':
                ch = L'が';
                break;
            case L'き':
                ch = L'ぎ';
                break;
            case L'く':
                ch = L'ぐ';
                break;
            case L'け':
                ch = L'げ';
                break;
            case L'こ':
                ch = L'ご';
                break;

            case L'が':
                ch = L'か';
                break;
            case L'ぎ':
                ch = L'き';
                break;
            case L'ぐ':
                ch = L'く';
                break;
            case L'げ':
                ch = L'け';
                break;
            case L'ご':
                ch = L'こ';
                break;

            // はひふへほ

            case L'は':
                ch = L'ば';
                break;
            case L'ひ':
                ch = L'び';
                break;
            case L'ふ':
                ch = L'ぶ';
                break;
            case L'へ':
                ch = L'べ';
                break;
            case L'ほ':
                ch = L'ぼ';
                break;

            case L'ば':
                ch = L'ぱ';
                break;
            case L'び':
                ch = L'ぴ';
                break;
            case L'ぶ':
                ch = L'ぷ';
                break;
            case L'べ':
                ch = L'ぺ';
                break;
            case L'ぼ':
                ch = L'ぽ';
                break;

            case L'ぱ':
                ch = L'は';
                break;
            case L'ぴ':
                ch = L'ひ';
                break;
            case L'ぷ':
                ch = L'ふ';
                break;
            case L'ぺ':
                ch = L'へ';
                break;
            case L'ぽ':
                ch = L'ほ';
                break;

            // さしすせそ

            case L'さ':
                ch = L'ざ';
                break;
            case L'し':
                ch = L'じ';
                break;
            case L'す':
                ch = L'ず';
                break;
            case L'せ':
                ch = L'ぜ';
                break;
            case L'そ':
                ch = L'ぞ';
                break;

            case L'ざ':
                ch = L'さ';
                break;
            case L'じ':
                ch = L'し';
                break;
            case L'ず':
                ch = L'す';
                break;
            case L'ぜ':
                ch = L'せ';
                break;
            case L'ぞ':
                ch = L'そ';
                break;

            // たちつてと

            case L'た':
                ch = L'だ';
                break;
            case L'ち':
                ch = L'ぢ';
                break;
            case L'つ':
                ch = L'づ';
                break;
            case L'て':
                ch = L'で';
                break;
            case L'と':
                ch = L'ど';
                break;

            case L'だ':
                ch = L'た';
                break;
            case L'ぢ':
                ch = L'ち';
                break;
            case L'づ':
                ch = L'つ';
                break;
            case L'で':
                ch = L'て';
                break;
            case L'ど':
                ch = L'と';
                break;

            // vu

            case L'う':
                ch = L'ゔ';
                break;
            case L'ゔ':
                ch = L'う';
                break;
        }

        if (is_katakana)
            ch += L'ア' - L'あ';

        // for the lulz

        switch (ch) {
            case L'ワ':
                ch = L'ヷ';
                break;
            case L'ヰ':
                ch = L'ヸ';
                break;
            case L'ヱ':
                ch = L'ヹ';
                break;
            case L'ヲ':
                ch = L'ヺ';
                break;

            case L'ヷ':
                ch = L'ワ';
                break;
            case L'ヸ':
                ch = L'ヰ';
                break;
            case L'ヹ':
                ch = L'ヱ';
                break;
            case L'ヺ':
                ch = L'ヲ';
                break;
        }

        input[input_len - 1] = ch;
    }
}

void input_buffer::small()
{
    if (input_len > 0) {
        wchar_t ch = input[input_len - 1];

        bool is_katakana;

        if (ch >= 12448 && ch <= 12543) {
            // katakana
            is_katakana = true;
            ch += L'あ' - L'ア';
        } else {
            is_katakana = false;
        }

        switch (ch) {
            case L'あ':
                ch = L'ぁ';
                break;
            case L'い':
                ch = L'ぃ';
                break;
            case L'う':
                ch = L'ぅ';
                break;
            case L'え':
                ch = L'ぇ';
                break;
            case L'お':
                ch = L'ぉ';
                break;
            case L'や':
                ch = L'ゃ';
                break;
            case L'ゆ':
                ch = L'ゅ';
                break;
            case L'よ':
                ch = L'ょ';
                break;
            case L'つ':
                ch = L'っ';
                break;

            case L'ぁ':
                ch = L'あ';
                break;
            case L'ぃ':
                ch = L'い';
                break;
            case L'ぅ':
                ch = L'う';
                break;
            case L'ぇ':
                ch = L'え';
                break;
            case L'ぉ':
                ch = L'お';
                break;
            case L'ゃ':
                ch = L'や';
                break;
            case L'ゅ':
                ch = L'ゆ';
                break;
            case L'ょ':
                ch = L'よ';
                break;
            case L'っ':
                ch = L'つ';
                break;
        }

        if (is_katakana)
            ch += L'ア' - L'あ';

        input[input_len - 1] = ch;
    }
}

void input_buffer::reset(const wchar_t *initial_value)
{
    xwcscpy(input, initial_value);
    input_len = xwcslen(input);
}

class keyboard_layout
{
public:
    keyboard_layout(const g2d::texture *texture, const wchar_t *key_rows[]);

    void draw(int selected_key) const;
    int find_key_for(float x, float y) const;

private:
    struct key
    {
        float x, y;
        int code;
    };

    const g2d::texture *texture_;
    key *keys;
    int num_keys, num_rows;
};

keyboard_layout::keyboard_layout(const g2d::texture *texture, const wchar_t *key_rows[])
    : texture_(texture)
    , num_keys(0)
    , num_rows(0)
{
    // count # of keys and rows

    for (const wchar_t **row_ptr = key_rows; *row_ptr; row_ptr++) {
        for (const wchar_t *p = *row_ptr; *p; p++) {
            if (*p != 1)
                ++num_keys;
        }

        ++num_rows;
    }

    // initialize keys

    keys = new key[num_keys];

    float y = num_rows * KEY_HEIGHT;
    key *q = keys;

    for (const wchar_t **row_ptr = key_rows; *row_ptr; row_ptr++) {
        const wchar_t *row = *row_ptr;

        int row_width = 0;

        for (const wchar_t *p = row; *p; p++) {
            if (*p != NIL_KEY) {
                const key_info *ki = char_to_key[*p];
                row_width += ki->width;
            } else {
                row_width += KEY_WIDTH;
            }
        }

        float x = .5 * (window_width - row_width);

        for (const wchar_t *p = row; *p; p++) {
            if (*p != NIL_KEY) {
                const key_info *ki = char_to_key[*p];

                q->x = x;
                q->y = y;
                q->code = *p;
                ++q;

                x += ki->width;
            } else {
                x += KEY_WIDTH;
            }
        }

        y -= KEY_HEIGHT;
    }
}

void keyboard_layout::draw(int selected_key) const
{
    render::set_blend_mode(blend_mode::ALPHA_BLEND);
    render::set_color({1.f, 1.f, 1.f, 1.f});

    for (const key *p = keys; p != &keys[num_keys]; p++) {
        const key_info *ki = char_to_key[p->code];

        const int width = ki->width;

        const float du = ki->du;
        const float dv = ki->dv;

        const float u = ki->u + (p->code != selected_key ? 0 : du);
        const float v = ki->v;

        render::draw_quad(
                texture_,
                {{p->x, p->y}, {p->x + width, p->y}, {p->x + width, p->y - KEY_HEIGHT}, {p->x, p->y - KEY_HEIGHT}},
                {{u, v}, {u + du, v}, {u + du, v + dv}, {u, v + dv}},
                KEYBOARD_LAYER);
    }
}

int keyboard_layout::find_key_for(float x, float y) const
{
    for (const key *p = keys; p != &keys[num_keys]; p++) {
        const key_info *ki = char_to_key[p->code];

        if (x >= p->x && x < p->x + ki->width && y <= p->y && y > p->y - KEY_HEIGHT)
            return p->code;
    }

    return -1;
}

void draw_message(const wchar_t *text)
{
    ::draw_message(get_ortho_projection(), 1, text);
}

} // namespace

class hiscore_input_state_impl : public leaderboard_event_listener
{
public:
    hiscore_input_state_impl();

    void reset();
    void redraw() const;
    void update(uint32_t dt);
    void on_touch_down(float x, float y);
    void on_touch_up();
    void on_touch_move(float x, float y);
    void on_back_key();
    void on_menu_key();

    void set_score(int score);

    void on_check_hiscore_response(bool ok, bool is_hiscore) override;

private:
    void back_to_hiscore_list();
    void back_to_main_menu();
    void draw_input() const;

    bool touch_is_down_;
    int cur_selected_key_;
    const g2d::texture *keyboard_texture_;
    keyboard_layout *abc_keyboard_upper_;
    keyboard_layout *abc_keyboard_lower_;
    keyboard_layout *hiragana_keyboard_;
    keyboard_layout *katakana_keyboard_;
    keyboard_layout *cur_keyboard_;
    input_buffer input_area_;
    int score_;
    score_text score_text_;
#ifdef FIX_ME
    g2d::draw_queue text_;
#endif

    enum state
    {
        STATE_NONE,
        STATE_CHECKING_HISCORE,
        STATE_INPUT,
    } state_;
};

hiscore_input_state_impl::hiscore_input_state_impl()
    : keyboard_texture_(g2d::load_texture("images/keyboard.png"))
    , score_text_(get_font(font::large))
    , state_(STATE_NONE)
{
    static const struct key_code_width
    {
        wchar_t code;
        int width;
    } keys[] = {
        {'A', 1},          {'B', 1},
        {'C', 1},          {'D', 1},
        {'E', 1},          {'F', 1},
        {'G', 1},          {'H', 1},
        {'I', 1},          {'J', 1},
        {'K', 1},          {'L', 1},
        {'M', 1},          {'N', 1},
        {'O', 1},          {'P', 1},
        {'Q', 1},          {'R', 1},
        {'S', 1},          {'T', 1},
        {'U', 1},          {'V', 1},
        {'W', 1},          {'X', 1},
        {'Y', 1},          {'Z', 1},

        {'a', 1},          {'b', 1},
        {'c', 1},          {'d', 1},
        {'e', 1},          {'f', 1},
        {'g', 1},          {'h', 1},
        {'i', 1},          {'j', 1},
        {'k', 1},          {'l', 1},
        {'m', 1},          {'n', 1},
        {'o', 1},          {'p', 1},
        {'q', 1},          {'r', 1},
        {'s', 1},          {'t', 1},
        {'u', 1},          {'v', 1},
        {'w', 1},          {'x', 1},
        {'y', 1},          {'z', 1},

        {'0', 1},          {'1', 1},
        {'2', 1},          {'3', 1},
        {'4', 1},          {'5', 1},
        {'6', 1},          {'7', 1},
        {'8', 1},          {'9', 1},

        {L'あ', 1},        {L'い', 1},
        {L'う', 1},        {L'え', 1},
        {L'お', 1},        {L'か', 1},
        {L'き', 1},        {L'く', 1},
        {L'け', 1},        {L'こ', 1},
        {L'さ', 1},        {L'し', 1},
        {L'す', 1},        {L'せ', 1},
        {L'そ', 1},        {L'た', 1},
        {L'ち', 1},        {L'つ', 1},
        {L'て', 1},        {L'と', 1},
        {L'な', 1},        {L'に', 1},
        {L'ぬ', 1},        {L'ね', 1},
        {L'の', 1},        {L'は', 1},
        {L'ひ', 1},        {L'ふ', 1},
        {L'へ', 1},        {L'ほ', 1},
        {L'ま', 1},        {L'み', 1},
        {L'む', 1},        {L'め', 1},
        {L'も', 1},        {L'や', 1},
        {L'ゆ', 1},        {L'よ', 1},
        {L'ら', 1},        {L'り', 1},
        {L'る', 1},        {L'れ', 1},
        {L'ろ', 1},        {L'わ', 1},
        {L'ゐ', 1},        {L'ゑ', 1},
        {L'を', 1},        {L'ん', 1},

        {L'ア', 1},        {L'イ', 1},
        {L'ウ', 1},        {L'エ', 1},
        {L'オ', 1},        {L'カ', 1},
        {L'キ', 1},        {L'ク', 1},
        {L'ケ', 1},        {L'コ', 1},
        {L'サ', 1},        {L'シ', 1},
        {L'ス', 1},        {L'セ', 1},
        {L'ソ', 1},        {L'タ', 1},
        {L'チ', 1},        {L'ツ', 1},
        {L'テ', 1},        {L'ト', 1},
        {L'ナ', 1},        {L'ニ', 1},
        {L'ヌ', 1},        {L'ネ', 1},
        {L'ノ', 1},        {L'ハ', 1},
        {L'ヒ', 1},        {L'フ', 1},
        {L'ヘ', 1},        {L'ホ', 1},
        {L'マ', 1},        {L'ミ', 1},
        {L'ム', 1},        {L'メ', 1},
        {L'モ', 1},        {L'ヤ', 1},
        {L'ユ', 1},        {L'ヨ', 1},
        {L'ラ', 1},        {L'リ', 1},
        {L'ル', 1},        {L'レ', 1},
        {L'ロ', 1},        {L'ワ', 1},
        {L'ヰ', 1},        {L'ヱ', 1},
        {L'ヲ', 1},        {L'ン', 1},

        {L'!', 1},         {L'?', 1},
        {L'~', 1},         {L'-', 1},
        {L'#', 1},         {L'@', 1},
        {L'★', 1},         {L'☆', 1},
        {L'♪', 1},         {L'❤', 1},

        {SHIFT_KEY, 1},    {MARU_TENTEN_KEY, 1},
        {SMALL_KEY, 1},

        {' ', 2},  // space
        {'\n', 2}, // ok
        {ABC_KEY, 2},      {HIRAGANA_KEY, 2},
        {KATAKANA_KEY, 2}, {'\r', 2},
#ifdef FIX_ME
        {-1, -1},
#endif
    };

    // load texture

    const int texture_width = keyboard_texture_->get_pixmap_width();
    const int texture_height = keyboard_texture_->get_pixmap_height();

    const float key_du = static_cast<float>(KEY_WIDTH) / texture_width;
    const float key_dv = static_cast<float>(KEY_HEIGHT) / texture_height;

    // initialize key info

    int x = 0;
    int y = 0;

    for (const key_code_width *p = keys; p->code != -1; p++) {
        int width = KEY_WIDTH * p->width;

        if (x + 2 * width > texture_width) {
            x = 0;
            y += KEY_HEIGHT;
        }

        key_info *ki = new key_info;

        ki->u = static_cast<float>(x) / texture_width;
        ki->v = static_cast<float>(y) / texture_height;

        ki->du = key_du * p->width;
        ki->dv = key_dv;

        ki->width = width;

        char_to_key.insert(std::make_pair(p->code, ki));

        x += 2 * width;
    }

    // initialize layouts

    // static const char *layout[] = { "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM", "\t
    // \n", 0 };

    static const wchar_t *abc_keys_upper[] = {L"!?~-#@★☆♪❤", L"QWERTYUIOP", L"ASDFGHJKL",
                                              L"\x4ZXCVBNM", L"\x3 \r\n",   nullptr};

    static const wchar_t *abc_keys_lower[] = {L"1234567890", L"qwertyuiop", L"asdfghjkl",
                                              L"\x4zxcvbnm", L"\x3 \r\n",   nullptr};

    static const wchar_t *hiragana_keys[] = {L"わらやまはなたさかあ",
                                             L"ゐり\x01みひにちしきい",
                                             L"ゑるゆむふぬつすくう",
                                             L"をれ\x01めへねてせけえ",
                                             L"んろよもほのとそこお",
                                             L"\x6\x5\x7 \r\n",
                                             nullptr};

    static const wchar_t *katakana_keys[] = {L"ワラヤマハナタサカア",
                                             L"ヰリ\x01ミヒニチシキイ",
                                             L"ヱルユムフヌツスクウ",
                                             L"ヲレ\x01メヘネテセケエ",
                                             L"ンロヨモホノトソコオ",
                                             L"\x2\x5\x7 \r\n",
                                             nullptr};

    abc_keyboard_upper_ = new keyboard_layout(keyboard_texture_, abc_keys_upper);
    abc_keyboard_lower_ = new keyboard_layout(keyboard_texture_, abc_keys_lower);
    hiragana_keyboard_ = new keyboard_layout(keyboard_texture_, hiragana_keys);
    katakana_keyboard_ = new keyboard_layout(keyboard_texture_, katakana_keys);

    const float y_input_area = .6 * window_height;

#ifdef FIX_ME
    auto font = g2d::load_font("fonts/small");
    text_.text_program(get_program_instance<program_texture_uniform_color>().get_raw())
        .translate(.5 * window_width, y_input_area + 240)
        .align_center()
        .render_text(font, L"New high score!")
        .translate(0, -(240 - 48))
        .render_text(font, L"Please enter your name:");
#endif
}

void hiscore_input_state_impl::reset()
{
    touch_is_down_ = false;
    cur_selected_key_ = -1;
    input_area_.reset(cur_options->player_name ? cur_options->player_name : L"");
    cur_keyboard_ = abc_keyboard_lower_;

    state_ = STATE_NONE;

    total_tics = 0;
}

void hiscore_input_state_impl::set_score(int score)
{
    score_ = score;

#ifdef FIX_ME
    state_ = STATE_CHECKING_HISCORE;

    if (!get_net_leaderboard().async_check_hiscore(this, score_))
        back_to_main_menu();
#else
    score_text_.initialize(score);
    state_ = STATE_INPUT;
#endif
}

void hiscore_input_state_impl::draw_input() const
{
    cur_keyboard_->draw(cur_selected_key_);
    input_area_.draw();

    render::push_matrix();
    render::translate(.5 * window_width, .6 * window_height + 150);
    score_text_.draw(1);
    render::pop_matrix();

#ifdef FIX_ME
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

    auto &prog = get_program_instance<program_texture_uniform_color>();
    prog.use();
    prog.set_proj_modelview_matrix(mat);
    prog.set_texture(0);
    prog.set_color(g2d::rgba(1, 1, 1, 1));

    text_.draw();
#endif
}

void hiscore_input_state_impl::redraw() const
{
#if 0
	glClear(GL_COLOR_BUFFER_BIT);
#else
    get_prev_state()->redraw(); // draw main menu background
#endif

    // set_ortho_projection();

    switch (state_) {
        case STATE_CHECKING_HISCORE:
            draw_message(L"checking score...");
            break;

        case STATE_INPUT:
            draw_input();
            break;

        default:
            assert(0);
    }
}

void hiscore_input_state_impl::update(uint32_t dt)
{
    get_prev_state()->update(dt); // update main menu background
    total_tics += dt;
}

void hiscore_input_state_impl::on_touch_down(float x, float y)
{
    if (state_ == STATE_INPUT) {
        touch_is_down_ = true;
        cur_selected_key_ = cur_keyboard_->find_key_for(x, y);
    }
}

void hiscore_input_state_impl::on_touch_up()
{
    if (state_ == STATE_INPUT) {
        touch_is_down_ = false;

        if (cur_selected_key_ != -1) {
            switch (cur_selected_key_) {
                case '\n':
                    if (input_area_.get_input_len() > 0) {
                        cur_options->set_player_name(input_area_.get_input());
                        get_net_leaderboard().async_insert_hiscore(this, input_area_.get_input(), score_);
                        back_to_hiscore_list();
                        return;
                    } else {
                        break;
                    }

                case '\r':
                    input_area_.del();
                    break;

                case HIRAGANA_KEY:
                    cur_keyboard_ = hiragana_keyboard_;
                    break;

                case KATAKANA_KEY:
                    cur_keyboard_ = katakana_keyboard_;
                    break;

                case ABC_KEY:
                    cur_keyboard_ = abc_keyboard_lower_;
                    break;

                case SHIFT_KEY:
                    cur_keyboard_ = cur_keyboard_ == abc_keyboard_lower_ ? abc_keyboard_upper_ : abc_keyboard_lower_;
                    break;

                case MARU_TENTEN_KEY:
                    input_area_.maru_tenten();
                    break;

                case SMALL_KEY:
                    input_area_.small();
                    break;

                default:
                    input_area_.append_char(cur_selected_key_);
                    break;
            }
        }

        cur_selected_key_ = -1;
    }
}

void hiscore_input_state_impl::on_touch_move(float x, float y)
{
    if (state_ == STATE_INPUT) {
        if (touch_is_down_)
            cur_selected_key_ = cur_keyboard_->find_key_for(x, y);
    }
}

void hiscore_input_state_impl::on_menu_key()
{
}

void hiscore_input_state_impl::on_back_key()
{
}

void hiscore_input_state_impl::back_to_main_menu()
{
    auto *main_menu = static_cast<main_menu_state *>(get_prev_state());
    main_menu->show_background();
    main_menu->show_root_menu();
    pop_state();
}

void hiscore_input_state_impl::back_to_hiscore_list()
{
    pop_state();
    start_hiscore_list();
}

void hiscore_input_state_impl::on_check_hiscore_response(bool ok, bool is_hiscore)
{
    fprintf(stderr, "on_check_hiscore_response: %d/%d\n", ok, is_hiscore);

    if (!ok) {
        // failed to check highscore for some reason, back to menu
        back_to_main_menu();
    } else {
        if (is_hiscore) {
            score_text_.initialize(score_);
            state_ = STATE_INPUT;
        } else {
            // not a hiscore, back to menu
            back_to_main_menu();
        }
    }
}

hiscore_input_state::hiscore_input_state()
    : impl_(new hiscore_input_state_impl)
{
}

hiscore_input_state::~hiscore_input_state()
{
    delete impl_;
}

void hiscore_input_state::reset()
{
    impl_->reset();
}

void hiscore_input_state::redraw() const
{
    impl_->redraw();
}

void hiscore_input_state::update(uint32_t dt)
{
    impl_->update(dt);
}

void hiscore_input_state::on_touch_down(float x, float y)
{
    impl_->on_touch_down(x, y);
}

void hiscore_input_state::on_touch_up()
{
    impl_->on_touch_up();
}

void hiscore_input_state::on_touch_move(float x, float y)
{
    impl_->on_touch_move(x, y);
}

void hiscore_input_state::on_back_key()
{
    impl_->on_back_key();
}

void hiscore_input_state::on_menu_key()
{
    impl_->on_menu_key();
}

void hiscore_input_state::set_score(int score)
{
    impl_->set_score(score);
}
