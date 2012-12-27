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

void to_pgm(const grid<elev_type> & g, size_t sz, std::ostream & os) {
    size_t rows = std::min(g.rows(), sz);
    size_t cols = std::min(g.cols(), sz);
    os << "P5\n" << cols << ' ' << rows << " 255\n";
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            elev_type scaled = (g.get(c, r) - elev_min) / (elev_max - elev_min) * 256;
            os << static_cast<char>(scaled);
        }
    }
}

void to_ascii(const grid<elev_type> & g, size_t sz, std::ostream & os) {
    // http://paulbourke.net/dataformats/asciiart/
    //std::string greys = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
    std::string greys = " `'.,-=+rconue$$%%##@@";
    for (size_t r = 0; r < std::min(sz, g.rows()); ++r) {
        for (size_t c = 0; c < std::min(sz, g.cols()); ++c) {
            elev_type scaled = (g.get(c, r) - elev_min) / (elev_max - elev_min) * greys.size();
            scaled = std::max(elev_type(0), scaled);
            os << greys[std::min(greys.size()-1, static_cast<size_t>(scaled))];
        }
        os << '\n';
    }
}

int main(int argc, char ** argv) {
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("ascii", "output ascii (default)")
        ("pgm", "output pgm")
        ("size", po::value<size_t>(), "terrain size (default 32)")
        ("roughness", po::value<double>(), "terrain roughness (default 1.0)")
        ("seed", po::value<size_t>(), "random seed")
    ;
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    } catch (const boost::program_options::unknown_option & e) {
        std::cout << e.what() << '\n'
            << desc << '\n';
        return 1;
    }
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << '\n';
        return 1;
    }

    size_t size = 32;
    if (vm.count("size")) size = vm["size"].as<size_t>();
    bool ascii = true;
    bool pgm = false;
    if (vm.count("pgm")) ascii = false, pgm = true;
    double roughness = 1.0;
    if (vm.count("roughness")) roughness = vm["roughness"].as<double>();
    size_t seed = static_cast<size_t>(boost::chrono::high_resolution_clock::now().time_since_epoch().count());
    if (vm.count("seed")) seed = vm["seed"].as<size_t>();

    std::cerr << "Seed: " << seed << std::endl;
    map m(2, 2, seed);

    size_t exp = 0;
    size_t sz = 2;
    double wiggle = roughness;
    while (sz < size) {
        exp++;
        sz = 2*sz - 1;
        wiggle *= 2;
    }

    for (size_t i = 0; i < exp; ++i, wiggle /= 2) m = m.elaborate(wiggle);
    const grid<elev_type> & g = m.get_heights();
    if (pgm)
        to_pgm(g, size, std::cout);
    if (ascii)
        to_ascii(g, size, std::cout);
    return 0;
}
