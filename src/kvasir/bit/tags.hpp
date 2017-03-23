#pragma once
/**************************************************************************
Copyright 2015 Odin Holmes
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
****************************************************************************/
namespace kvasir{
namespace Tag {
	struct User {};
	struct None {};
	namespace Adc {
		template<int I>
		struct channel{ static constexpr int value = I; };
		static constexpr channel<0> channel0{};
		static constexpr channel<1> channel1{};
		static constexpr channel<2> channel2{};
		static constexpr channel<3> channel3{};
		static constexpr channel<4> channel4{};
		static constexpr channel<5> channel5{};
		static constexpr channel<6> channel6{};
		static constexpr channel<7> channel7{};
		static constexpr channel<8> channel8{};
		static constexpr channel<9> channel9{};
		static constexpr channel<10> channel10{};
		static constexpr channel<11> channel11{};
		static constexpr channel<12> channel12{};
		static constexpr channel<13> channel13{};
		static constexpr channel<14> channel14{};
		static constexpr channel<15> channel15{};
	}
	namespace Capture {
		template<int I>
		struct channel{ static constexpr int value{I}; };
		static constexpr channel<0> c0{};
		static constexpr channel<1> c1{};
		static constexpr channel<2> c2{};
		static constexpr channel<3> c3{};
		static constexpr channel<4> c4{};
		static constexpr channel<5> c5{};
		static constexpr channel<6> c6{};
		static constexpr channel<7> c7{};
		static constexpr channel<8> c8{};
		static constexpr channel<9> c9{};
		static constexpr channel<10> c10{};
		static constexpr channel<11> c11{};
		static constexpr channel<12> c12{};
		static constexpr channel<13> c13{};
		static constexpr channel<14> c14{};
		static constexpr channel<15> c15{};
	}
	namespace Match{
		template<int I>
		struct channel{ static constexpr int value{I}; };
		static constexpr channel<0> m0{};
		static constexpr channel<1> m1{};
		static constexpr channel<2> m2{};
		static constexpr channel<3> m3{};
		static constexpr channel<4> m4{};
		static constexpr channel<5> m5{};
		static constexpr channel<6> m6{};
		static constexpr channel<7> m7{};
		static constexpr channel<8> m8{};
		static constexpr channel<9> m9{};
		static constexpr channel<10> m10{};
		static constexpr channel<11> m11{};
		static constexpr channel<12> m12{};
		static constexpr channel<13> m13{};
		static constexpr channel<14> m14{};
		static constexpr channel<15> m15{};
	}
	namespace detail{
		template<typename T>
		struct Ischannel{
			static constexpr bool value = false;
		};
		template<int I>
		struct Ischannel<Adc::channel<I>>{
			static constexpr bool value = true;
		};
		template<typename T>
		constexpr int getchannelValue(){
			static_assert(Ischannel<T>::value,"expected a Tags::Adc::channel");
			return T::value;
		}
	}
}
}
