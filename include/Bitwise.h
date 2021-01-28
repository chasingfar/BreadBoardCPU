#ifndef UTIL_BITWISE_H
#define UTIL_BITWISE_H

#include "bitset.h"
#include <climits>
#include <utility>
#include <type_traits>
#include <iostream>
#include <sstream>

inline static int is_logging = 0;
inline static std::string log_prefix = ":";
#define LOG(...) if(is_logging!=0){std::cout<<log_prefix<<__func__;print_arg(std::cout,__VA_ARGS__);++is_logging;log_prefix=":";}
#define LOG_START() is_logging = 1;log_prefix=":";
#define LOG_END() log_prefix=" | :";
#define LOG_STOP() is_logging = -1;
/*std::make_tuple(__VA_ARGS__);
#include <tuple>
template<class TupType, size_t... I>
std::ostream& tuple_printer(std::ostream& os,const TupType& _tup, std::index_sequence<I...>){
	return (..., (os << (I == 0? "" : ", ") << std::get<I>(_tup)));
}
template<typename... T>
std::ostream& operator<<(std::ostream& os, const std::tuple<T...>& value) {
	return os<<"("<<tuple_printer(os,value, std::make_index_sequence<sizeof...(T)>())<<")";
}*/
template<typename T0,typename... T>
std::ostream& print_arg(std::ostream& os,T0 v0, T... value){
	return os<<"("<<v0<<((os << ',' << value), ...,")");
}

namespace Util {

	namespace Bitwise {
		template<typename T, size_t N = sizeof(T) * CHAR_BIT>
		constexpr inline static size_t BitSize = N;
		template<size_t N>
		constexpr inline static size_t BitSize<std::bitset<N> > = N;

		/**
		 *
		 *
		 */
		constexpr std::size_t BitWidth(std::size_t x) {
			if (x == 0) {
				return 0;
			}
			std::size_t i = 1, v = x;
			while (v >>= 1) {
				++i;
			}
			return i;
		};

		/**
		 * fill N of bit with 1 as mask
		 * ex.
		 * BitMaskFill<uint8_t,5> == 0b00011111
		 * BitMaskFill<uint8_t>   == 0b11111111
		 *
		 * @param T type of value
		 * @param N of 1
		 * @return unsigned value use as mask
		 */
		template<typename T, size_t N = BitSize<T>>
		const inline static T BitMaskFill = (~T(0)) >> (BitSize<T> - N);

		/**
		 * mask to use in BitReverse
		 * ex.
		 * BitMaskSplit<uint8_t,2,true > == 0b11001100
		 * BitMaskSplit<uint8_t,2,false> == 0b00110011
		 * BitMaskSplit<uint8_t,1,false> == 0b01010101
		 *
		 * @param T type of value
		 * @param num_of_mask_bits number of 1 per group
		 * @param HIGH set if want 1 on high bits
		 * @return unsigned value use as mask
		 */
		template<class T, std::size_t num_of_mask_bits, bool HIGH>
		constexpr T BitMaskSplit = [] {
			std::size_t N = sizeof(T) * CHAR_BIT;
			//generate mask like 0b00000011
			T mask = BitMaskFill<T, num_of_mask_bits>;
			//fill with mask to 0b00110011
			for (std::size_t shift = (num_of_mask_bits << 1); shift < N; shift <<= 1) {
				mask |= mask << shift;
			}
			return HIGH ? ~mask : mask;
		}();

		/**
		 * bitwise reverse an integer value
		 * ex.
		 * BitReverse((uint8_t)0b11010100) == 0b00101011
		 * note : the bit size of v should be 2^n n=0,1,2,...
		 * @param v unsigned value to reverse
		 * @return bitwise reversed value
		 */
		template<class T, std::size_t N , std::size_t ...I >
		constexpr T BitReverse_impl(T v, std::index_sequence<I...>) {
			return (
				(v = (
					(v & BitMaskSplit<T, 1 << (N - I - 1), true>)
					>> (1 << (N - I - 1))
				) | (
					(v & BitMaskSplit<T, 1 << (N - I - 1), false>)
				    << (1 << (N - I - 1))
				)), ...
			, v);
		}
		/*
		uint32_t bit_reverse(uint32_t x) {
            uint32_t n = x;
            n = ((n & 0xffff0000) >> 16) | ((n & 0x0000ffff) << 16);
            n = ((n & 0xff00ff00) >>  8) | ((n & 0x00ff00ff) <<  8);
            n = ((n & 0xf0f0f0f0) >>  4) | ((n & 0x0f0f0f0f) <<  4);
            n = ((n & 0xcccccccc) >>  2) | ((n & 0x33333333) <<  2);
            n = ((n & 0xaaaaaaaa) >>  1) | ((n & 0x55555555) <<  1);
            return n;
        }
		*/
		template<class T, std::size_t N = BitWidth(BitSize<T>) - 1>
		constexpr T BitReverse(const T v) {
			return BitReverse_impl(v, std::make_index_sequence<N>{});
		}

		/**
		 * bitwise reverse an integer value
		 * ex.
		 * BitReverse((uint8_t)0b11010100) == 0b00101011
		 * note : the bit size of v should be 2^n n=0,1,2,...
		 * @param v unsigned value to reverse
		 * @return bitwise reversed value
		 */
		template<class T, std::size_t N , std::size_t ...I >
		constexpr T BitCount_impl(T v, std::index_sequence<I...>) {
			return (
				(v = (
				     v & BitMaskSplit<T, 1 << I, false>
			    ) + (
					(v >> (1 << I))
					& BitMaskSplit<T, 1 << I, false>
				)), ...
			, v);
		}
		template<class T, std::size_t N = BitWidth(BitSize<T>) - 1>
		constexpr T BitCount(const T v) {
			return BitCount_impl(v, std::make_index_sequence<N>{});
		}
		/*
		uint32_t bit_count(uint32_t v) {
		v = (v & 0x55555555) + ((v >> 1) & 0x55555555);
		v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
		v = (v & 0x0F0F0F0F) + ((v >> 4) & 0x0F0F0F0F);
		v = (v & 0x00FF00FF) + ((v >> 8) & 0x00FF00FF);
		v = (v & 0x0000FFFF) + ((v >> 16) & 0x0000FFFF);
		return v;
		}
		*/

		/**
		 * create a pair of getter and setter bitwise field
		 * @param N size in bits of field
		 * @param P previous BitField type or use StartAt<S> assign
		 */
		namespace BitField {

			template<size_t S>
			struct StartAt{
				static constexpr size_t low = S;
				static constexpr size_t high = S;
			};

			enum class FollowMode{
				outerHigh,
				outerLow,
				innerHigh,
				innerLow,
			};

			template<size_t Size, typename Ref = StartAt<0> , FollowMode followMode=FollowMode::outerHigh>
			struct BitField {
				using type = unsigned long long;//bitset_wrap<Size>;//
				static constexpr size_t size = Size;
				static constexpr size_t low =
						followMode == FollowMode::outerHigh ? (Ref::high) :
						followMode == FollowMode::outerLow ? (Ref::low - Size) :
						followMode == FollowMode::innerHigh ? (Ref::high - Size) :
						(Ref::low);
				static constexpr size_t high = low + size;

				template<typename T, typename U>
				static T set(T o, U v) {
					o ^= ((T(v) & BitMaskFill<T, size>)^get(o)) << low;
					return o;
				}
				template<typename nRef,typename T, typename U>
				static T setAs(T o, U v) {
					o ^= ((T(v) & BitMaskFill<T, nRef::size>)^get(o)) << low-nRef::low;
					return o;
				}

				template<typename T>
				static T get(const T o) {
					return (o >> low) & BitMaskFill<T, size>;
				}
				template<typename T,typename U>
				static T getAs(const U o) {
					return static_cast<T>(get(o));
				}
			};
			template<auto value,typename BaseField>
			struct BitId:BaseField{
				static constexpr auto id=value;
				template<typename T=typename BaseField::type >
				static T set(T o=0u){
					return BaseField::set(o,id);
				}
				template<typename nRef,typename T=typename BaseField::type >
				static T setAs(T o=0u){
					return BaseField::template setAs<nRef>(o,id);
				}
				template<typename T>
				static bool test(T o) {
					return BaseField::get(o)==T(id);
				}
			};

#define BITFILEDBASE(defaultSize) template<size_t Size = defaultSize, typename Ref = StartAt<0> , FollowMode followMode=FollowMode::outerHigh,typename Base = BitField<Size,Ref,followMode> >
		} // namespace BitField
	} // namespace Bitwise
} // namespace Util
#endif //UTIL_BITWISE_H