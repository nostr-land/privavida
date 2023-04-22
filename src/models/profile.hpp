//
//  profile.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once
#include "event.hpp"

//const content = {
//  "banner": "https://mdnt30.com/images/mandelbro-banner.jpg",
//  "website":"https://mdnt30.com/mandelbro/system-change",
//  "lud06":"",
//  "nip05":"mandl.mp3@mdnt30.com",
//  "picture":"https://mdnt30.com/images/mandelbro-profile.jpg",
//  "display_name":"mandl.mp3🎵",
//  "about":"indie rock band hailing from paris\n\nnew single \"System Change\" out on all platforms",
//  "name":"mandl.mp3",
//  "lud16":"lnbc1pjrh5uppp5p8ds5dd63vnymfh4ymujkrt589ly9j837wx4e3p04uuv5fjfpnnqdqu2askcmr9wssx7e3q2dshgmmndp5scqzpgxqyz5vqsp5m47e8ss4hkz0pjcxlkeakx5fapf77e0ggcl2sel84qc3gj6rpsnq9qyyssqfdv24cucfphqf92ekt2t26s24kdlwfve8gx8kkjyw354nj0xr085cfy8nk7cexqhl7z6yv6tdtzxjfp8mjcd73h5m6yxyl9849kzx9spcymx9j"
//};

//
/// Profile
//
struct Profile {
    static const uint8_t VERSION = 0x01;

    // Profile header (contains version number & size)
    uint32_t __header__;

    Pubkey  pubkey;
    EventId event_id;

    // Profile properties
    RelString name;
    RelString display_name;
    RelString picture;
    RelString website;
    RelString banner;
    RelString nip05;
    RelString about;
    RelString lud16;

    uint8_t  __buffer[];

    static uint32_t size_of(const Profile* profile) {
        return (0x00FFFFFF & profile->__header__);
    }
    static uint32_t size_from_event(const Event* event) {
        return sizeof(Profile) + event->content.size;
    }
    static uint8_t version_number(const Profile* profile) {
        return (0xFF000000 & profile->__header__) >> 24;
    }
};

bool parse_profile_data(Profile* profile, const Event* event);
