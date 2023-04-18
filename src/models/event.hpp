//
//  event.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 18/07/2018.
//

#pragma once
#include <inttypes.h>
#include "relative.hpp"

enum EventValidState {
    EVENT_NOT_CHECKED = 0,
    EVENT_VALID,
    EVENT_INVALID_ID,
    EVENT_INVALID_SIG
};
enum EventContentEncryptionState {
    EVENT_CONTENT_NOT_CHECKED = 0,
    EVENT_CONTENT_REGULAR,
    EVENT_CONTENT_ENCRYPTED,
    EVENT_CONTENT_DECRYPTED,
    EVENT_CONTENT_DECRYPT_FAILED
};

//
/// Event
//
//  An Event struct has a dynamic size, where all data
//  pertaining to the event is stored off the end
//  inside the __buffer[] array.
//
//  As all the pointers in the data structure are
//  relative pointers, meaning that the Event data
//  can be trivially copied/moved and serialized.
//
//
struct Event {
    static const uint8_t VERSION = 0x01;

    // Event header (contains version number & size)
    uint32_t __header__;

    // Regular note data
    uint32_t  kind;
    uint64_t  created_at;
    uint8_t   id[32];
    uint8_t   pubkey[32];
    uint8_t   sig[64];
    RelString content;
    RelArray<RelArray<RelString>> tags;

    // Derived data
    EventValidState validity;
    EventContentEncryptionState content_encryption;

    uint8_t  __buffer[];

    static uint32_t size_of(const Event* event) {
        return (0x00FFFFFF & event->__header__);
    }
    static uint8_t version_number(const Event* event) {
        return (0xFF000000 & event->__header__) >> 24;
    }
};
