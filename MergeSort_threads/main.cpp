#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <atomic>
#include <ctime>

static std::atomic<size_t> TCounter{0};
static std::atomic<size_t> MaxTCounter{8};


template<typename Iter>
void MergeSortThread(Iter begin, Iter end){
    if(end - begin <= 1){
        return;
    }
    std::vector<std::thread> TVector;

    if(MaxTCounter > TCounter && ((end - begin) > 5000000)){
        ++TCounter;
        TVector.emplace_back([&](){
            MergeSortThread(begin, begin + (end - begin) / 2);
        });

        if(MaxTCounter > TCounter && ((end - begin) > 5000000)) {
            ++TCounter;
            TVector.emplace_back([&]() {
                MergeSortThread(begin + (end - begin) / 2, end);
            });
        }
        else{
            MergeSortThread(begin + (end - begin) / 2, end);
        }
    }
    else{
        MergeSortThread(begin, begin + (end - begin) / 2);
        MergeSortThread(begin + (end - begin) / 2, end);
    }


    for(auto &th : TVector){
        th.join();
        --TCounter;
    }


    Merge(begin, begin + (end - begin) / 2, begin + (end - begin) / 2, end);

}

template<typename T>
void BubbleSort(std::vector<T> &a){
    size_t N = a.size();
    bool flag = true;

    for(size_t i = 0; i < a.size() && flag; ++i){
        --N;
        flag = false;
        for(size_t j = 0; j < N; ++j){

            if(a[j] > a[j+1]){
                std::swap(a[j+1], a[j]);
                flag = true;
            }
        }
    }
}

template<typename T>
void InsertionSort(std::vector<T> &a){
    size_t N = a.size();
    for(size_t i = 0; i < N; ++i){
        for(size_t j = i; j > 0 ; --j){
            if(a[j] < a[j-1]){
                std::swap(a[j], a[j-1]);
            }
            else{
                break;
            }
        }
    }

}

template<typename Iter>
typename std::iterator_traits<Iter>::value_type Partition(Iter begin, Iter end){
    return*(begin + (end - begin) / 2);
}

template<typename Iter>
void QSort(Iter begin, Iter end){
    if(end - begin <= 1){
        return;
    }
    Iter it1 = begin;
    Iter it2 = end-1;
    auto p = Partition(begin, end);

    while(it1 < it2){
        for(size_t i = 0; i < end-begin; ++i){std::cout << *(begin+i) << ' ';}
        std::cout <<"next " << *it1 << " " << p << " " << *it2 << std::endl;
        if(*it1 > p && *it2 < p){
           std::swap(*it1, *it2);
           ++it1;
           --it2;
        }
        else {
            if(*it1 > p || *it2 == p){
                --it2;
            } else{
                if(*it2 < p || *it1 == p){
                    ++it1;
                }
                else {
                    ++it1;
                    --it2;
                }
            }
        }

    }

    QSort(begin, it2);
    QSort(it1, end);
}


template<typename Iter>
void Merge(Iter begin1, Iter end1, Iter begin2, Iter end2){
    using v_type = typename std::iterator_traits<Iter>::value_type;
    std::vector<v_type> New;
    New.reserve(end2 - begin1);
    Iter it1 = begin1;
    Iter it2 = begin2;
    while(it1 < end1 && it2 < end2){
        if(*it1 < *it2){
            New.emplace_back(std::move(*it1));
            ++it1;
        }
        else {
            New.emplace_back(std::move(*it2));
            ++it2;
        }
    }
    while(it1 < end1){
        New.emplace_back(std::move(*it1));
        ++it1;
    }
    while(it2 < end2){
        New.emplace_back(std::move(*it2));
        ++it2;
    }
    auto new_it = New.begin();
    while ( begin1 != end2) {
        *begin1 = std::move(*new_it);
        ++begin1;
        ++new_it;
    }

}
template<typename Iter>
void MergeSort(Iter begin, Iter end){
    if(end - begin <= 1){
        return;
    }
    MergeSort(begin, begin + (end - begin) / 2);
    MergeSort(begin + (end - begin) / 2, end);
    Merge(begin, begin + (end - begin) / 2, begin + (end - begin) / 2, end);
}


int main() {
    std::vector<int> b{1,5,6,11,1,2};
    //int *a1 = new int[5];
    //a1[0] = 5;
    //a1[1] = 2;
    //a1[2] = 31;
    //a1[3] = 1;
    std::vector<std::vector<int>> a;
    std::vector<int> VEC;
    size_t sizer = 100;
    for(size_t i = 0; i < 1; ++i){a.push_back(VEC);}
    for(size_t j = 0; j < 1; ++j){
        for(size_t i = 0; i < sizer; ++i){
            a[j].emplace_back(sizer-i);
        }
    }

    std::vector<double> Timer;
    double time_elapsed_ms;
    for(size_t i = 0; i < 1; ++i){
        std::clock_t c_start = std::clock();
        //std::cout << c_start;
        //BubbleSort(a[i]);
        //InsertionSort(a[i]);
        //MergeSort(a[i].begin(), a[i].end());
        //QSort(a[i].begin(), a[i].end());
        //MergeSortThread(a[i].begin(), a[i].end());
        std::clock_t c_end = std::clock();
        //std::cout << c_end;
        time_elapsed_ms = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
        std::cout << "CPU time used: " << time_elapsed_ms << " ms\n";
        Timer.push_back(time_elapsed_ms);
    }
    for(size_t i = 0; i < 1; ++i){std::cout << Timer[i] << ' ';}
    std::ofstream myfile;
    myfile.open ("example.txt");
    for(double i : Timer){
        myfile << i << std::endl;
    }

    myfile.close();
    //for(size_t i = 0; i < a[0].size(); ++i){std::cout << a[0][i] << ' ';}
    QSort(b.begin(), b.end());
    for(size_t i = 0; i < b.size(); ++i){std::cout << b[i] << ' ';}

    return 0;
}
