#ifdef members
#ifdef THIS
#ifdef BASE

	using This = THIS;
	using Base = BASE;
	DEF_TYPE0

	#define M(name) name##_ ,
	    enum {members};
	#undef M

	#define M(name) SubType<name##_>::type name{};
		members
	#undef M

	#define M(name) ,name(get<name##_>())
		template<typename T> requires std::is_base_of_v<MemVar,T>
		explicit THIS(std::shared_ptr<T> value):Base{value}
			members
			{}
		explicit THIS(Allocator& allocator):Base{allocator.alloc(size)}
			members
			{}
		THIS(const Base& base):Base{base}
			members
			{}
	#undef M

#undef BASE
#endif
#undef THIS
#endif
#undef members
#endif