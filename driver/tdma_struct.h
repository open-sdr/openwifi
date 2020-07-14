#define MAXIMUM_USER 30

struct LOWADDRESS_AID {
    u32 lowaddress; //as key
    u16 aid;
};

struct AID_TSF {
    u16 aid; // as key
    u64 tsf;
};

struct INDEX_AID
{
    u16 index; //as key
    u16 aid;
};

u64 get_hash_u32(u32 key)
{
    return (key % ((u32)MAXIMUM_USER));
}

u16 get_hash_u16(u16 key)
{
    return (key % ((u16) MAXIMUM_USER));
}

int get_hash_int(int key)
{
    return (key % ((int)MAXIMUM_USER));
}