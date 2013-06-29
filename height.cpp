// vim:set sw=4 et sts=4:
#include <random>
#include <vector>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <boost/chrono/include.hpp>
#include <boost/program_options.hpp>

typedef float elev_type;
const float elev_min = 0.0;
const float elev_max = 1000.0;

template <typename T>
class grid {
    size_t m_rows;
    size_t m_cols;
    std::vector<T> data;

public:
    grid(size_t cols, size_t rows)
        : m_rows(rows)
        , m_cols(cols)
        , data(cols * rows)
    {
    }

    T & get(size_t x, size_t y) {
        return data[y*m_cols+x];
    }

    const T & get(size_t x, size_t y) const {
        return data[y*m_cols+x];
    }

    size_t rows() const { return m_rows; }
    size_t cols() const { return m_cols; }
};

class height_rng {
    std::mt19937 engine;
    std::uniform_real_distribution<elev_type> dist;

public:
    height_rng(size_t seed)
        : engine(seed)
    {
    }

    elev_type operator()() {
        return in_range(elev_min, elev_max);
    }

    elev_type operator()(elev_type mean, elev_type radius) {
        elev_type min = std::max(mean-radius, elev_min);
        elev_type max = std::min(min+2*radius, elev_max);
        return in_range(min, max);
    }

    size_t gen_seed() {
        return engine();
    }

private:
    elev_type in_range(elev_type min, elev_type max) {
        std::uniform_real_distribution<elev_type> d(min, max);
        dist.param(d.param());
        return dist(engine);
    }
};

class map {
    grid<elev_type> heights;
    height_rng rng;

    struct noinit_tag {};
    static const noinit_tag noinit;

    map(size_t width, size_t length, size_t seed, noinit_tag)
        : heights(length, width)
        , rng(seed)
    {
    }

public:
    map(size_t width, size_t length, size_t seed)
        : heights(length, width)
        , rng(seed)
    {
        for (size_t r = 0; r < length; ++r) {
            for (size_t c = 0; c < width; ++c) {
                heights.get(r, c) = rng();
            }
        }
    }

    const grid<elev_type> & get_heights() const { return heights; }

private:
    elev_type avg(elev_type a, elev_type b) {
        return a + (b-a)/2;
    }

    elev_type f(double wiggle, elev_type a, elev_type b) {
        return rng(avg(a, b), wiggle);
    }

    elev_type f(double wiggle, elev_type a, elev_type b, elev_type c, elev_type d) {
        return rng(avg(avg(a, b), avg(c, d)), wiggle);
    }

public:
    map elaborate(const double wiggle) {
        const double w = wiggle;

        map res(2*heights.cols()-1, 2*heights.rows()-1, rng.gen_seed(), noinit);

        const grid<elev_type> & input = heights;
        grid<elev_type> & output = res.heights;

        const elev_type * in = &input.get(0, 0);

        // First row
        {
            elev_type * out = &output.get(0, 0);
            elev_type prev = *out++ = *in++;
            for (size_t c = 1; c < input.cols(); ++c) {
                *out++ = f(w, prev, *in);
                prev = *out++ = *in++;
            }
        }

        for (size_t r = 1; r < input.rows(); ++r) {
            // Row 2r in output
            {
                auto above = &output.get(0, 2*r-1);
                auto out = &output.get(0, 2*r);
                elev_type prev = *above++ = *out++ = *in++;
                for (size_t c = 1; c < input.cols(); ++c) {
                    *above++ = *out++ = f(w, prev, *in);
                    prev = *above++ = *out++ = *in++;
                }
            }

            // Row 2r-1 in output
            elev_type * prev = &output.get(0, 2*r-2);
            elev_type * cur = &output.get(0, 2*r-1);
            // *cur temporarily contains the value from below
            *cur = f(w, *prev, *cur);
            for (size_t c = 1; c < input.cols(); ++c) {
                ++prev;
                elev_type above = *prev++;
                elev_type left = *cur++;
                elev_type * mid = cur++;

                // neighbourhood. Y = computed, N = to compute
                //        2c-2 2c-1  2c
                // 2r-2 [  Y    Y   pY  ]   p = prev pointer
                // 2r-1 [  Y  *mN  *cN  ]   c = cur pointer, m = mid pointer
                // 2r   [  Y    Y    Y  ]   * = contains data from below

                *cur = f(w, *prev, *cur);
                *mid = f(w, above, left, *mid, *cur);
            }
        }

        return res;
    }
};

struct dims {
    size_t width;
    size_t length;
};

template <typename Rounded>
class floyd_steinberg {
    std::vector<elev_type> m_cur;
    std::vector<elev_type> m_next;

    size_t m_idx;
    size_t m_range;

public:
    floyd_steinberg(size_t width, size_t range)
        : m_cur(width)
        , m_next(width)
        , m_idx(0)
        , m_range(range)
    {
    }

    Rounded round_next(elev_type e) {
        Rounded r = closest(e + m_cur[m_idx]);

        elev_type error = e - inverse(r);

        if (m_idx+1 < m_cur.size()) {
            m_cur[m_idx+1] += error * 7/16;
            m_next[m_idx+1] += error * 1/16;
        }

        if (m_idx > 0)
            m_next[m_idx-1] += error * 3/16;

        m_next[m_idx] += error * 5/16;

        m_idx = (m_idx + 1) % m_cur.size();
        if (m_idx == 0) {
            std::swap(m_cur, m_next);
            std::fill(m_next.begin(), m_next.end(), 0.0);
        }

        return r;
    }

private:
    Rounded closest(elev_type e) {
        elev_type scaled
            = (e - elev_min) / (elev_max - elev_min) * m_range;
        scaled = std::max(elev_type(0), scaled);
        return static_cast<Rounded>(std::min(m_range-1,
                                             static_cast<size_t>(scaled)));
    }

    elev_type inverse(Rounded r) {
        return elev_min + (elev_max - elev_min) * static_cast<size_t>(r) / m_range;
    }
};

void to_pgm(const grid<elev_type> & g, dims sz, std::ostream & os) {
    size_t rows = std::min(g.rows(), sz.length);
    size_t cols = std::min(g.cols(), sz.width);
    floyd_steinberg<unsigned char> fs(cols, 256);
    os << "P5\n" << cols << ' ' << rows << " 255\n";
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            os << fs.round_next(g.get(c, r));
        }
    }
}

void to_ascii(const grid<elev_type> & g, dims sz, std::ostream & os) {
    // http://paulbourke.net/dataformats/asciiart/
    //std::string greys = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
    std::string greys = " `'.,-+=rcoea$$%%##@@";
    size_t rows = std::min(g.rows(), sz.length);
    size_t cols = std::min(g.cols(), sz.width);
    floyd_steinberg<size_t> fs(cols, greys.size());
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            os << greys[fs.round_next(g.get(c, r))];
        }
        os << '\n';
    }
}

grid<elev_type> lightmap(const grid<elev_type> & g, double lval) {
    grid<elev_type> res(g.cols(), g.rows());
    const double PI = acos(0);
    const double sun_angle = -PI/4;
    const double cos_sun = cos(sun_angle);
    const double sin_sun = sin(sun_angle);
    for (size_t r = 1; r < g.rows()-1; ++r) {
        for (size_t c = 1; c < g.cols()-1; ++c) {
            elev_type range = elev_max - elev_min;
            elev_type v = g.get(c+1, r) - g.get(c-1, r);
            elev_type u = g.get(c, r-1) - g.get(c, r+1);
            double light = (-v*cos_sun - u*sin_sun) / range * lval;
            double tanlight = atan(light);
            elev_type z = static_cast<elev_type>((PI/2 + tanlight)/PI * (elev_max-elev_min) + elev_min);
            res.get(c, r) = z;
        }
    }
    return res;
}

dims parse_dims(std::string input) {
    const char * errmsg = "Invalid size; must be <W>x<L> or a simple integer";
    auto x = std::find(input.begin(), input.end(), 'x');
    if (x == input.end()) {
        char * p = 0;
        size_t i = strtol(input.c_str(), &p, 10);
        if (*p != '\0') throw std::invalid_argument(errmsg);
        return {.width = i, .length = i};
    }
    *x = '\0';
    ++x;
    char * p = 0;
    size_t w = strtol(&input[0], &p, 10);
    if (*p != '\0') throw std::invalid_argument(errmsg);
    size_t l = strtol(&*x, &p, 10);
    if (*p != '\0') throw std::invalid_argument(errmsg);
    return {.width = w, .length = l};
}

int main(int argc, char ** argv) {
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("ascii", "output ascii (default)")
        ("pgm", "output pgm")
        ("size", po::value<std::string>(), "terrain size (default 32)")
        ("roughness", po::value<double>(), "terrain roughness (default 1.0)")
        ("seed", po::value<size_t>(), "random seed")
        ("light", po::value<double>(), "produce a light map")
    ;
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    } catch (const boost::program_options::unknown_option & e) {
        std::cout << e.what() << '\n'
            << desc << '\n';
        return 1;
    } catch (const boost::program_options::invalid_option_value & e) {
        std::cout << e.what() << '\n' << desc << '\n';
        return 1;
    } catch (const boost::program_options::invalid_command_line_syntax & e) {
        std::cout << e.what() << '\n' << desc << '\n';
        return 1;
    }
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << '\n';
        return 1;
    }

    dims d{.width = 32, .length = 32};
    if (vm.count("size")) d = parse_dims(vm["size"].as<std::string>());
    d.width = std::min(std::numeric_limits<size_t>::max()/2, d.width);
    d.length = std::min(std::numeric_limits<size_t>::max()/2, d.length);
    bool ascii = true;
    bool pgm = false;
    if (vm.count("pgm")) ascii = false, pgm = true;
    double roughness = 1.0;
    if (vm.count("roughness")) roughness = vm["roughness"].as<double>();
    size_t seed = static_cast<size_t>(boost::chrono::high_resolution_clock::now().time_since_epoch().count());
    if (vm.count("seed")) seed = vm["seed"].as<size_t>();
    bool light = false;
    double lval = 0;
    if (vm.count("light")) light = true, lval = vm["light"].as<double>();

    std::cerr << "Seed: " << seed << std::endl;
    map m(2, 2, seed);

    size_t exp = 0;
    size_t sz = 2;
    double wiggle = roughness;
    size_t size = std::max(d.width, d.length);
    while (sz < size) {
        exp++;
        sz = 2*sz - 1;
        wiggle *= 2;
    }

    for (size_t i = 0; i < exp; ++i, wiggle /= 2) {
        try {
            m = m.elaborate(wiggle);
        } catch (std::bad_alloc) {
            std::cerr << "Out of memory; aborting expansion" << std::endl;
        }
    }
    const grid<elev_type> & g = m.get_heights();
    if (light) {
        grid<elev_type> l = lightmap(g, lval);
        if (pgm)
            to_pgm(l, d, std::cout);
        if (ascii)
            to_ascii(l, d, std::cout);
    } else {
        if (pgm)
            to_pgm(g, d, std::cout);
        if (ascii)
            to_ascii(g, d, std::cout);
    }
    return 0;
}
