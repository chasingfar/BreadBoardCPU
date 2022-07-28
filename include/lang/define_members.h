#define M(name) name##_ ,
    enum {members};
#undef M
#define M(name) auto name(){return get<name##_>();}
	members
#undef M
#undef members