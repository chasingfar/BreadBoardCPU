//
// Created by chasingfar on 2021/2/14.
//
#ifndef ENUM_BASE
#define ENUM_BASE unsigned
#endif
#ifndef ENUM_OTHER
#define ENUM_OTHER ;
#endif
#ifndef ENUM_HIDE_TYPE
#define ENUM_HIDE_TYPE namespace
#endif

#define TMP_NAME ENUM_NAME(_TMP)
#define OUT_NAME ENUM_NAME()
ENUM_HIDE_TYPE _enum_x_detail {
	struct TMP_NAME {
		using base_t = ENUM_BASE;
		enum struct Value : base_t {
#define X(a) a,
			ENUM_TABLE
#undef X
		};
		inline static const std::string strings[]={
#define X(a) #a,
			ENUM_TABLE
#undef X
		};
		Value value;

		TMP_NAME () = default;
		explicit constexpr TMP_NAME(Value value) :value(value) {}
		explicit constexpr TMP_NAME(base_t value) : value(static_cast<Value>(value)) {}

		constexpr operator Value() const { return value; }
		explicit operator bool() = delete;        // Prevent usage: if(fruit)

		constexpr base_t v() const { return static_cast<base_t>(value); }
		template<typename Integer,std::enable_if_t<std::is_integral<Integer>::value, bool> = true>
		explicit constexpr operator Integer() const { return v(); }

		std::string str() const { return strings[v()]; }
		explicit operator std::string() const { return str(); }
		friend std::ostream &operator<<(std::ostream &os, TMP_NAME reg) {
			return os << std::string(reg);
		}

		ENUM_OTHER
	};
};
struct OUT_NAME : _enum_x_detail::TMP_NAME {
	typedef typename _enum_x_detail::TMP_NAME tmp_base_t;
	using tmp_base_t::tmp_base_t;
	using tmp_base_t::v;
#define X(a) static constexpr tmp_base_t a{tmp_base_t::Value::a};
	ENUM_TABLE
#undef X
	constexpr OUT_NAME(tmp_base_t value) :tmp_base_t(value) {}

};
#undef TMP_NAME
#undef OUT_NAME
#undef ENUM_NAME
#undef ENUM_TABLE
#undef ENUM_BASE
#undef ENUM_OTHER
#undef ENUM_HIDE_TYPE