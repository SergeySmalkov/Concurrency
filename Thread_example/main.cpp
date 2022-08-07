#include "iostream"
#include "fstream"
#include "string"
#include "vector"
#include "thread"

void generate_and_write_quotes () {

    for( size_t i = 0; i < 10; ++i) {
        std::ofstream outFile;
        std::cout << "started writing " + std::to_string(i) << std::endl;
        outFile.open("sample.txt");
        outFile << "10:0" + std::to_string(i)  + ' '  + "55." + std::to_string(i) << std::endl;
        outFile << "10:0" + std::to_string(i)  + ' '  + "55.1" + std::to_string(i) << std::endl;
        outFile << "10:0" + std::to_string(i)  + ' '  + "55.2" + std::to_string(i) << std::endl;
        outFile.close();
        std::cout << "ended writing " + std::to_string(i) << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

};

std::pair<std::vector<std::string>, std::vector<double>> read_quotes (std::ifstream& inFile) {
    inFile.open("sample.txt");
    std::string item;
    std::vector<std::string> time;
    std::vector<double> quotes;

    while (!inFile.eof()) {
        inFile >> item;
        if (item[2] == ':') {
            time.emplace_back(item);
        }
        else{
            quotes.emplace_back(std::stod(item));
        }
    }
    quotes.pop_back();
    return {time, quotes};
}

double average(std::vector<double>& vec) {
    double avg = 0;
    for (auto& i : vec) {
        avg += i;
    }
    return avg / vec.size();
}

void reading() {
    for( size_t i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "started reading " + std::to_string(i) << std::endl;
        std::ifstream inFile;
        std::pair<std::vector<std::string>, std::vector<double>> prices = read_quotes(inFile);
        for (size_t i = 0; i < prices.first.size(); ++i) {
            std::cout << prices.first[i] << std::endl;
            std::cout << prices.second[i] << std::endl;
        }
        std::cout << "ended reading " + std::to_string(i) << std::endl;
        std::cout << "VWAP: " << average(prices.second) << std::endl;
    }
}

int main()
{
    std::thread th1(generate_and_write_quotes);
    std::thread th2(reading);
    th1.join();
    th2.join();
    return 0;
}
