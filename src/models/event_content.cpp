//
//  event_content.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-28.
//

#include "event_content.hpp"

typedef EventContentToken Token;

struct Cursor {
    const char* ch;
    const char* start;
    const char* end;
};

struct ParserContext {
    ParserContext(const Event* event, StackArray<Token>& tokens, StackArray<NostrEntity*>& entities, StackBuffer& data)
        : event(event), tokens(tokens), entities(entities), data(data), data_allocated(0) {}

    const Event* event;
    StackArray<Token>& tokens;
    StackArray<NostrEntity*>& entities;
    StackBuffer& data;
    size_t data_allocated;
};

static bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

static bool is_boundary(char c) {
    return !isalnum(c);
}

static bool is_invalid_url_ending(char c) {
    return c == '!' || c == '?' || c == ')' || c == '.' || c == ',' || c == ';';
}

static bool is_alphanumeric(char c) {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

static bool consume_until_boundary(Cursor* cur) {
    char c;
    
    while (cur->ch < cur->end) {
        c = *cur->ch;
        
        if (is_boundary(c))
            return true;
        
        cur->ch++;
    }
    
    return true;
}

static bool consume_until_whitespace(Cursor* cur, bool or_end) {
    char c;
    bool consumed_at_least_one = false;
    
    while (cur->ch < cur->end) {
        c = *cur->ch;
        
        if (is_whitespace(c)) return consumed_at_least_one;
        
        cur->ch++;
        consumed_at_least_one = true;
    }
    
    return or_end;
}

static int consume_until_non_alphanumeric(Cursor* cur, bool or_end) {
    char c;
    bool consumed_at_least_one = false;

    while (cur->ch < cur->end) {
        c = *cur->ch;

        if (!is_alphanumeric(c)) return consumed_at_least_one;

        cur->ch++;
        consumed_at_least_one = true;
    }

    return or_end;
}

static bool parse_char(Cursor* cur, char c) {
    if (cur->ch >= cur->end)
        return false;
        
    if (*cur->ch == c) {
        cur->ch++;
        return true;
    }
    
    return false;
}

static int peek_char(Cursor* cur, int ind) {
    if ((cur->ch + ind < cur->start) || (cur->ch + ind >= cur->end))
        return -1;

    return *(cur->ch + ind);
}

static bool parse_digit(Cursor* cur, int* digit) {
    int c;
    if ((c = peek_char(cur, 0)) == -1)
        return false;
    
    c -= '0';
    
    if (c >= 0 && c <= 9) {
        *digit = c;
        cur->ch++;
        return true;
    }
    return false;
}

static int parse_str(Cursor* cur, const char* str) {
    int i;
    char c, cs;
    unsigned long len;

    len = strlen(str);
    
    if (cur->ch + len >= cur->end)
        return false;

    for (i = 0; i < len; i++) {
        c = tolower(cur->ch[i]);
        cs = tolower(str[i]);
        
        if (c != cs)
            return false;
    }

    cur->ch += len;

    return true;
}

static RelString make_rel_string(ParserContext& ctx, const char* start, const char* end) {
    uint32_t offset = (uint32_t)(start - (const char*)ctx.event);
    uint32_t size = (uint32_t)(end - start);
    return RelString(size, offset);
}

static void add_text_token(Cursor* cur, ParserContext& ctx, const char* end) {
    if (cur->start < end) {
        Token& token = ctx.tokens.push_back();
        token.type = Token::TEXT;
        token.text = make_rel_string(ctx, cur->start, end);
    }
}

static bool parse_nip08_mention(Cursor* cur, ParserContext& ctx) {
    int d1, d2, d3, ind;
    auto start = cur->ch;

    if (!parse_str(cur, "#["))
        return false;

    if (!parse_digit(cur, &d1)) {
        cur->ch = start;
        return false;
    }
    
    ind = d1;
    
    if (parse_digit(cur, &d2))
        ind = (d1 * 10) + d2;
    
    if (parse_digit(cur, &d3))
        ind = (d1 * 100) + (d2 * 10) + d3;
    
    if (!parse_char(cur, ']')) {
        cur->ch = start;
        return false;
    }

    add_text_token(cur, ctx, start);
    Token& token = ctx.tokens.push_back();
    token.type = Token::NIP08_MENTION;
    token.text = make_rel_string(ctx, start, cur->ch);
    token.nip08_mention_index = ind;
    return true;
}

static bool parse_hashtag(Cursor* cur, ParserContext& ctx) {
    int c;
    auto start = cur->ch;
    
    if (!parse_char(cur, '#'))
        return false;
    
    c = peek_char(cur, 0);
    if (c == -1 || is_whitespace(c) || c == '#') {
        cur->ch = start;
        return false;
    }
    
    consume_until_boundary(cur);

    add_text_token(cur, ctx, start);
    Token& token = ctx.tokens.push_back();
    token.type = Token::HASHTAG;
    token.text = make_rel_string(ctx, start + 1, cur->ch);
    return true;
}

static bool parse_url(Cursor* cur, ParserContext& ctx) {
    auto start = cur->ch;

    if (!parse_str(cur, "http"))
        return false;

    if (parse_char(cur, 's') || parse_char(cur, 'S')) {
        if (!parse_str(cur, "://")) {
            cur->ch = start;
            return false;
        }
    } else {
        if (!parse_str(cur, "://")) {
            cur->ch = start;
            return false;
        }
    }

    if (!consume_until_whitespace(cur, true)) {
        cur->ch = start;
        return false;
    }

    // strip any unwanted characters
    while (is_invalid_url_ending(peek_char(cur, -1))) {
        --cur->ch;
    }

    add_text_token(cur, ctx, start);
    Token& token = ctx.tokens.push_back();
    token.type = Token::URL;
    token.text = make_rel_string(ctx, start, cur->ch);
    return true;
}

static bool parse_nostr_entity(Cursor* cur, ParserContext& ctx) {
    auto start = cur->ch;

    if (!parse_str(cur, "nostr:"))
        return false;

    auto entity_start = cur->ch;
    
    if (!consume_until_non_alphanumeric(cur, true)) {
        cur->ch = start;
        return false;
    }

    auto entity_len = (uint32_t)(cur->ch - entity_start);

    auto decoded_size = NostrEntity::decoded_size(entity_start, entity_len);
    if (decoded_size == -1) {
        cur->ch = start;
        return false;
    }

    ctx.data.reserve(ctx.data_allocated + decoded_size);
    auto entity = (NostrEntity*)((uint8_t*)ctx.data.data + ctx.data_allocated);
    if (!NostrEntity::decode(entity, entity_start, entity_len)) {
        cur->ch = start;
        return false;
    }
    ctx.data_allocated += NostrEntity::size_of(entity);
    ctx.entities.push_back(entity);

    add_text_token(cur, ctx, start);
    Token& token = ctx.tokens.push_back();
    token.type = Token::ENTITY;
    token.text = make_rel_string(ctx, start, cur->ch);

    return true;
}

void event_content_parse(const Event* event, StackArray<EventContentToken>& tokens, StackArray<NostrEntity*>& entities, StackBuffer& data) {
    ParserContext ctx(event, tokens, entities, data);

    Cursor cur;
    cur.ch    = event->content.data.get(event);
    cur.start = cur.ch;
    cur.end   = cur.ch + event->content.size;

    while (cur.ch < cur.end) {
        auto cp = peek_char(&cur, -1);
        auto c  = peek_char(&cur, 0);

        if (cp == -1 || is_whitespace(cp)) {
            if (c == '#' && (parse_nip08_mention(&cur, ctx) || parse_hashtag(&cur, ctx))) {
                cur.start = cur.ch;
                continue;
            } else if ((c == 'h' || c == 'H') && parse_url(&cur, ctx)) {
                cur.start = cur.ch;
                continue;
            } else if (c == 'n' && parse_nostr_entity(&cur, ctx)) {
                cur.start = cur.ch;
                continue;
            }
        }

        cur.ch++;
    }

    add_text_token(&cur, ctx, cur.end);
}

size_t event_content_size_needed_for_copy(const Event* event, const StackArray<EventContentToken>& tokens, const StackArray<NostrEntity*>& entities) {
    return (
        Event::size_of(event) +
        tokens.size * sizeof(EventContentToken) +
        (entities.size ? (uint8_t*)entities.back() - (uint8_t*)entities.front() + NostrEntity::size_of(entities.back()) : 0)
    );
}

void event_content_copy_result_into_event(Event* event, const StackArray<EventContentToken>& tokens, const StackArray<NostrEntity*>& entities) {

    uint32_t offset = Event::size_of(event);

    // Copy the content tokens over
    event->content_tokens = RelArray<EventContentToken>((uint32_t)tokens.size, offset);
    auto tokens_out = event->content_tokens.get(event);
    memcpy(&tokens_out[0], &tokens[0], tokens.size * sizeof(EventContentToken));
    offset += tokens.size * sizeof(EventContentToken);

    // Copy all the entities over
    int entity_index = 0;
    for (auto& token : tokens_out) {
        if (token.type != Token::ENTITY) continue;
        token.entity = RelPointer<NostrEntity>(offset);
        memcpy(token.entity.get(event), entities[entity_index], NostrEntity::size_of(entities[entity_index]));
        offset += NostrEntity::size_of(entities[entity_index]);
    }

    Event::set_size(event, offset);
}
