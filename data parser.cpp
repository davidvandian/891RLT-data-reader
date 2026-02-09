#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <limits>

#define INFD std::numeric_limits<double>::infinity()

struct intustring { int i; std::string s; };
struct data { double freq; double ohm; double deg; };

/*
static std::ifstream openfile(std::string& filedir) {
    std::ifstream filestream{ filedir };
    if (!filestream.is_open()) {
        throw std::ios_base::failure("File not open at " + filedir + '!');
    }
    return filestream;
}
*/

static bool ynq(const std::string& query) {
    char response{};
    while ((response != 'y') && (response != 'n')) {
        std::cout << '\n' << query << " (y/n) ";
        std::cin >> response;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if ((response != 'y') && (response != 'n')) {
            std::cout << "Response invalid; trying again.";
        }
    }
    return (response == 'y');
}

static void blackener(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {return !std::isspace(c); }).base(), s.end());
    std::replace(s.begin(), s.end(), ' ', '-');
}

static void fileinput(const std::string& type, std::filesystem::path& file, std::string extns = "") {
    while (true) {
        std::string input{};
        std::cout << "Input " << type << " file directory: ";
        std::getline(std::cin, input);
        std::erase(input, '\"');

        file = input;
        std::string ext{ file.extension().string() };
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (extns != "" && ext != '.' + extns) {
            std::cout << "Not a valid file! Try again: ";
            continue;
        }
        if (!std::filesystem::exists(file)) {
            std::cout << "File does not exist! Try again: ";
            continue;
        }
        else {
            std::cout << "File accepted :)\n\n";
            break;
        }
    }
}

static std::string cursornav(std::ifstream& filestream, const size_t lines, const size_t cols, const bool reset = true, const bool output = true) {
    if (reset) {
        filestream.clear();
        filestream.seekg(0);
    }
    
    if (!filestream) {
        throw std::runtime_error("File stream not good");
    }

    for (size_t i{ 1 }; i < lines; i++) {
        filestream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    std::string container{};

    filestream.ignore(cols);
    if (output) { std::getline(filestream, container); }

    if (filestream.eof()) { filestream.clear(); }
    if (filestream.fail()) { throw std::runtime_error("Reading comprehension issues!"); }

    return container;
}

static std::string parameters() {
    bool undone{ true };
    unsigned int thickness{};
    std::string layers{};
    std::string dielectric{};

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    while (undone) {
        char rubcount{ 0 };

        std::cout << "\nHow many um is the gap thickness between the plates? ";
        std::cin >> thickness;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        while (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "That's not a number!\nEnter a number!!! ";
            std::cin >> thickness;
        }
        
        bool response{ ynq("Is the glass ITO?") };
        std::cout << "\nSpecify the coating on each plate: ";
        std::getline(std::cin, layers);
        blackener(layers);
        if (layers != "") { layers += '-'; }
        if (response) { layers = "ITO-" + layers; }
		bool valid{ false };
        while (!valid) {
            std::cout << "\nHow many plates are rubbed? (input 'c' for a commercial cell) ";
            std::cin >> rubcount;
            while (std::cin.fail()) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Inputted data type is too wide!\nEnter a single character!!! ";
                std::cin >> rubcount;
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            switch (rubcount) {
			case '0': { layers += "unrubbed-unrubbed"; valid = true; break; }
			case '1': { layers += "rubbed-unrubbed"; valid = true; break; }
			case '2': { layers += "rubbed-rubbed"; valid = true; break; }
			case 'c': { layers += "commercial"; valid = true; break; }
			default: { std::cout << "Unrecognized input! Trying again...\n"; }
			}
        }

        std::cout << "\nWhat dielectric occupies the capacitor space? ";
        std::getline(std::cin, dielectric);
        blackener(dielectric);

        response = ynq("Are these all correct?");
        if (response) { undone = false; }
    }
    return std::to_string(thickness) + "um_" + layers + '_' + dielectric + '_';
}

static intustring preprocessing(std::ifstream& stream1, std::ifstream& stream2, std::string& type) {
    if (!stream1.is_open() || !stream2.is_open()) {
        throw std::runtime_error("At least one of the files is not open!");
    }

    std::string Zdate{ cursornav(stream1, 3, 12) };
    std::replace(Zdate.begin(), Zdate.end(), '/', '-');

    std::string Ztime{ cursornav(stream1, 4, 14) };
    std::replace(Ztime.begin(), Ztime.end(), ':', '.');

    const std::string Zlevel{ cursornav(stream1, 9, 13) };
    int Zsize{};
    try {
        Zsize = std::stoi(cursornav(stream1, 10, 13));
    }
    catch (const std::invalid_argument& e) {
        std::cerr << e.what();
    }

    const std::string θlevel{ cursornav(stream2, 9, 13) };
    int θsize{};
    try {
        θsize = std::stoi(cursornav(stream2, 10, 13));
    }
    catch (const std::invalid_argument& e) {
        std::cerr << e.what();
    }

    if (θlevel != Zlevel || θsize != Zsize) {
        std::cerr << "These datasets are incompatible with each other: ";
        if (θsize != Zsize) {
            std::cerr << "the number of data do not correspond.";
        }
        else if (θlevel != Zlevel) {
            std::cerr << "the RMS voltages do not correspond.";
        }
    }

    return { Zsize, '\\' + Zdate + '_' + Ztime + '_' + parameters() + Zlevel + '_' + type + ".csv"};
}

static double reader(std::ifstream& filestream, const size_t lines, const size_t cols, const size_t len, bool advance = false) {
    double result{};
    
    cursornav(filestream, lines, cols, false, false);
    std::string s{};
    for (size_t j{ 0 }; j < len; j++) {
        s.push_back(filestream.get());
    }
    blackener(s);

	if (s == "ver") { cursornav(filestream, 0, 2, false, false); return INFD; }
	
    result = std::stod(s);

    if (advance) { filestream.get(); }

    char OoM{ (char)filestream.peek() };
    switch (OoM) {
    case ('G'): { result *= 1e9; break; }
    case ('M'): { result *= 1e6; break; }
    case ('k'): { result *= 1e3; break; }
    case (' '):
    case ('d'): { break; }
    case ('m'): { result *= 1e-3; break; }
    case ('u'): { result *= 1e-6; break; }
    case ('n'): { result *= 1e-9; break; }
    case ('p'): { result *= 1e-12; break; }
    case ('f'): { result *= 1e-15; break; }
    default: { throw std::exception("Invalid data!"); }
    }

    return result;
}

int main() {
    std::filesystem::path dir{};
    bool isfirst{ true };
    while (true) {
        std::filesystem::path Zfiledir{};
        std::filesystem::path θfiledir{};

        std::string type;
        std::cout << "Input measurement type: ";
        std::getline(std::cin, type);
        std::cout << '\n' << std::flush;

        fileinput(type, Zfiledir, "txt");
        fileinput("theta", θfiledir, "txt");

        std::ifstream Zfile{ Zfiledir };
        std::ifstream θfile{ θfiledir };

        const auto [size, newfilename] = preprocessing(Zfile, θfile, type);

        std::vector<data> list{};

        cursornav(Zfile, 13, 0, true, false);
        cursornav(θfile, 13, 0, true, false);

        for (size_t i{ 0 }; i < (300 / size) + 1; ++i) {
            data datapoint{};

            datapoint.freq = reader(Zfile, 1, 11, 5);
            datapoint.ohm = reader(Zfile, 0, 12, 5, true);
            double θfreq{ reader(θfile, 1, 11, 5) };
            datapoint.deg = reader(θfile, 0, 11, 6);

            if (datapoint.freq != θfreq) { std::cerr << "These files have different frequencies and are thus incompatible\n"; }

            cursornav(Zfile, 1, 4, false, false);
            cursornav(θfile, 1, 5, false, false);

            if (datapoint.freq != INFD && datapoint.ohm != INFD && datapoint.deg != INFD) { list.push_back(datapoint); }
        }

        bool samedir{ ynq("Save result in same directory?") };
        if ((Zfiledir.parent_path() == θfiledir.parent_path()) && isfirst && samedir) {
            dir = Zfiledir.parent_path();
        }
        else if (!samedir) {
            std::filesystem::path ndir{};
            fileinput("result", ndir);
            while (std::filesystem::status(ndir).type() != std::filesystem::file_type::directory) {
                std::cout << "\nNot a folder! ";
                fileinput("again", ndir);
            }
            if (ndir == dir) { std::cout << "This is the same directory :) you just wasted time reinputting this :) :) :)"; }
            dir = ndir;
        }
        auto ldir{ dir };
        std::ofstream output{ ldir.concat(newfilename) };

        output << "Frequency (Hz)," + type + "-mag," + type + "-phase(deg)\n";

        for (auto& i : list) {
            output << i.freq << ',' << std::scientific << i.ohm << ',' << i.deg << '\n';
        }

        std::cout << "\nSaved to " << ldir << '\n';
        
        if (!ynq("\n---------\nProcess another file?")) { break; }

        isfirst = false;
    }

	std::cout << "\nPress Enter to exit...";
	std::cin.get();

    return 0;
}