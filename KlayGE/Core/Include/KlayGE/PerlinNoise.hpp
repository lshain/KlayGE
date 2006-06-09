// PerlinNoise.hpp
// KlayGE 数学函数库 头文件
// Ver 2.5.0
// 版权所有(C) 龚敏敏, 2005
// Homepage: http://klayge.sourceforge.net
// This algorithm is based on the great work done by Ken Perlin.
// http://mrl.nyu.edu/~perlin/doc/oscar.html
//
// 2.5.0
// 初次建立 (2005.4.11)
//
// 修改记录
///////////////////////////////////////////////////////////////////////////////

#ifndef _PERLINNOISE_HPP
#define _PERLINNOISE_HPP

#include <cstdlib>
#include <ctime>

#include <KlayGE/Math.hpp>

namespace KlayGE
{
	namespace MathLib
	{
		// noise functions over 1, 2, and 3 dimensions

		template <typename T>
		class PerlinNoise
		{
		public:
			static PerlinNoise& Instance()
			{
				static PerlinNoise instance;
				return instance;
			}

			T noise(T const & x)
			{
				Vector_T<int, 2> bx;
				Vector_T<T, 2> rx;

				this->setup(x, bx, rx);

				int i = p_[bx.x()];
				int j = p_[bx.y()];

				float sx = this->curve(rx.x());

				float u = rx.x() * g1_[i].x();
				float v = rx.y() * g1_[j].x();

				return lerp(u, v, sx);
			}

			T noise(T const & x, T const & y)
			{
				Vector_T<int, 2> bx, by;
				Vector_T<T, 2> rx, ry;

				this->setup(x, bx, rx);
				this->setup(y, by, ry);

				int i = p_[bx.x()];
				int j = p_[bx.y()];

				int b00 = p_[i + by.x()];
				int b01 = p_[i + by.y()];
				int b10 = p_[j + by.x()];
				int b11 = p_[j + by.y()];

				T sx = this->curve(rx.x());
				T sy = this->curve(ry.x());

				T a, b;
				{
					T u = dot(g2_[b00], float2(rx.x(), ry.x()));
					T v = dot(g2_[b10], float2(rx.y(), ry.x()));
					a = lerp(u, v, sx);
				}
				{
					T u = dot(g2_[b01], float2(rx.x(), ry.y()));
					T v = dot(g2_[b11], float2(rx.y(), ry.y()));
					b = lerp(u, v, sx);
				}

				return lerp(a, b, sy);
			}

			T noise(T const & x, T const & y, T const & z)
			{
				Vector_T<int, 2> bx, by, bz;
				Vector_T<T, 2> rx, ry, rz;

				this->setup(x, bx, rx);
				this->setup(y, by, ry);
				this->setup(z, bz, rz);

				int i = p_[bx.x()];
				int j = p_[bx.y()];

				int b00 = p_[i + by.x()];
				int b01 = p_[i + by.y()];
				int b10 = p_[j + by.x()];
				int b11 = p_[j + by.y()];

				int b000 = b00 + bz.x();
				int b001 = b00 + bz.y();
				int b010 = b01 + bz.x();
				int b011 = b01 + bz.y();
				int b100 = b10 + bz.x();
				int b101 = b10 + bz.y();
				int b110 = b11 + bz.x();
				int b111 = b11 + bz.y();

				T sx = this->curve(rx.x());
				T sy = this->curve(ry.x());
				T sz = this->curve(rz.x());

				T c, d;
				{
					T a, b;
					{
						T u = dot(g3_[b000], float3(rx.x(), ry.x(), rz.x()));
						T v = dot(g3_[b100], float3(rx.y(), ry.x(), rz.x()));
						a = lerp(u, v, sx);
					}
					{
						T u = dot(g3_[b010], float3(rx.x(), ry.y(), rz.x()));
						T v = dot(g3_[b110], float3(rx.y(), ry.y(), rz.x()));
						b = lerp(u, v, sx);
					}
					c = lerp(a, b, sy);
				}
				{
					T a, b;
					{
						T u = dot(g3_[b001], float3(rx.x(), ry.x(), rz.y()));
						T v = dot(g3_[b101], float3(rx.y(), ry.x(), rz.y()));
						a = lerp(u, v, sx);
					}
					{
						T u = dot(g3_[b011], float3(rx.x(), ry.y(), rz.y()));
						T v = dot(g3_[b111], float3(rx.y(), ry.y(), rz.y()));
						b = lerp(u, v, sx);
					}
					d = lerp(a, b, sy);
				}

				return lerp(c, d, sz);
			}

			T turbulence(T const & x, T const & y, T freq)
			{
				T t(0);

				do
				{
					t += this->noise(freq * x, freq * y) / freq;
					freq *= T(0.5);
				} while (freq >= 1);
				
				return t;
			}

			T turbulence(T const & x, T const & y, T const & z, T freq)
			{
				T t(0);

				do
				{
					t += this->noise(freq * x, freq * y, freq * z) / freq;
					freq *= T(0.5);
				} while (freq >= 1);
				
				return t;
			}

			T tileable_noise(T const & x, T const & w)
			{
				return (this->noise(x + 0) * (w - x)
					+ this->noise(x - w) * (0 + x)) / w;
			}

			T tileable_noise(T const & x, T const & y,
				T const & w, T const & h)
			{
				return (this->noise(x + 0, y + 0) * (w - x) * (h - y)
					+ this->noise(x - w, y + 0) * (0 + x) * (h - y)
					+ this->noise(x + 0, y - h) * (w - x) * (0 + y)
					+ this->noise(x - w, y - h) * (0 + x) * (0 + y)) / (w * h);
			}

			T tileable_noise(T const & x, T const & y, T const & z,
				T const & w, T const & h, T const & d)
			{
				return (this->noise(x + 0, y + 0, z + 0) * (w - x) * (h - y) * (d - z)
					+ this->noise(x - w, y + 0, z + 0) * (0 + x) * (h - y) * (d - z)
					+ this->noise(x + 0, y - h, z + 0) * (w - x) * (0 + y) * (d - z)
					+ this->noise(x - w, y - h, z + 0) * (0 + x) * (0 + y) * (d - z)
					+ this->noise(x + 0, y + 0, z - d) * (w - x) * (h - y) * (0 + z)
					+ this->noise(x - w, y + 0, z - d) * (0 + x) * (h - y) * (0 + z)
					+ this->noise(x + 0, y - h, z - d) * (w - x) * (0 + y) * (0 + z)
					+ this->noise(x - w, y - h, z - d) * (0 + x) * (0 + y) * (0 + z)) / (w * h * d);
			}

			T tileable_turbulence(T const & x, T const & y,
				T const & w, T const & h, T freq)
			{
				T t(0);

				do
				{
					t += this->tileable_noise(freq * x, freq * y, w * freq, h * freq) / freq;
					freq *= T(0.5);
				} while (freq >= 1);
				
				return t;
			}

			T tileable_turbulence(T const & x, T const & y, T const & z,
				T const & w, T const & h, T const & d, T freq)
			{
				T t(0);

				do
				{
					t += this->tileable_noise(freq * x, freq * y, freq * z, w * freq, h * freq, d * freq) / freq;
					freq *= T(0.5);
				} while (freq >= 1);
				
				return t;
			}

		private:
			static int const B = 0x1000;

			PerlinNoise()
			{
				std::srand(static_cast<unsigned int>(std::time(NULL)));

				for (int i = 0; i < B; ++ i)
				{
					p_[i] = i;

					for (int j = 0; j < 1; ++ j)
					{
						g1_[i][j] = static_cast<T>((std::rand() % (B + B)) - B) / B;
					}

					for (int j = 0; j < 2; ++ j)
					{
						g2_[i][j] = static_cast<T>((std::rand() % (B + B)) - B) / B;
					}
					g2_[i] = normalize(g2_[i]);

					for (int j = 0; j < 3; ++ j)
					{
						g3_[i][j] = static_cast<T>((std::rand() % (B + B)) - B) / B;
					}
					g3_[i] = normalize(g3_[i]);
				}

				for (int i = B - 1; i > 0; -- i)
				{
					std::swap(p_[i], p_[std::rand() % B]);
				}

				for (int i = 0; i < B + 2; ++ i)
				{
					p_[B + i] = p_[i];
					g1_[B + i] = g1_[i];
					g2_[B + i] = g2_[i];
					g3_[B + i] = g3_[i];
				}
			}

			void setup(T const & i, Vector_T<int, 2>& b, Vector_T<T, 2>& r)
			{
				int const BM = 0xFF;
				int const N = 0x1000;

				T t = i + N;
				b.x() = (static_cast<int>(t)) & BM;
				b.y() = (b.x() + 1) & BM;
				r.x() = t - static_cast<int>(t);
				r.y() = r.x() - 1;
			}

			T curve(T const & t)
			{
				return t * t * (3 - 2 * t);
			}

		private:
			int p_[B + B + 2];

			Vector_T<T, 1> g1_[B + B + 2];
			Vector_T<T, 2> g2_[B + B + 2];
			Vector_T<T, 3> g3_[B + B + 2];
		};
	}
}

#endif		// _PERLINNOISE_HPP
