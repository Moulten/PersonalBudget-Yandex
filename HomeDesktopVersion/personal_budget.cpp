#include "test_runner.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <map>

using namespace std;

static map<int,int> days_in_month = {
		    {1,31},{2,28},{3,31},{4,30},
			{5,31},{6,30},{7,31},{8,31},
			{9,30},{10,31},{11,30},{12,31}
	};
class Date{
public:
	explicit Date(string new_date){
		stringstream ss(new_date);
		  ss>>year_;
		  ss.ignore(1);
		  ss>>month_;
		  ss.ignore(1);
		  ss>>day_;
	}

	time_t AsTimestamp() const {
	  std::tm t;
	  t.tm_sec   = 0;
	  t.tm_min   = 0;
	  t.tm_hour  = 0;
	  t.tm_mday  = day_;
	  t.tm_mon   = month_ - 1;
	  t.tm_year  = year_ - 1900;
	  t.tm_isdst = 0;
	  return mktime(&t);
	}

	bool operator<(const Date& other) const{
		return this->AsTimestamp() < other.AsTimestamp();
	}
	void operator++(){
		day_++;
		if(day_ > days_in_month[month_]){
			day_ = 1;
			month_++;
		}
		if(month_ > 12){
			month_ = 1;
			year_++;
		}
	}
	string Print() const{
		stringstream ss;
		ss<<year_<<'-'<<month_<<'-'<<day_;
		return ss.str();
	}

private:
	int year_;
	int month_;
	int day_;
};

int ComputeDaysDiff(const Date& date_to, const Date& date_from) {
  const time_t timestamp_to = date_to.AsTimestamp();
  const time_t timestamp_from = date_from.AsTimestamp();
  static const int SECONDS_IN_DAY = 60 * 60 * 24;
  return (timestamp_to - timestamp_from) / SECONDS_IN_DAY;
}

//контейнер отрезков. одна дата хранит начало периода и стоимость каждого дня
//вторая дата хранит значение по умолчанию
//в случае, если часть диапазона умножается на процент, то создается новый диапазон внутри старого
template<typename K, typename V>
class Intervals {
public:
	struct Interval{
		K start;
		K end;
		V value;
	};

public:
	Intervals(V const& val)
	: m_valBegin(val)
	{}

	void assign( K const& keyBegin, K const& keyEnd, V const& val ) {
		if(keyBegin < keyEnd){
			auto left_bound = m_map.emplace(keyBegin,val);
			auto left_iter = left_bound.first;
			V right_value = left_bound.second ? m_valBegin : left_iter->second;
			if(!left_bound.second ){
				left_iter->second = val;
			}
			if (left_iter != m_map.begin()){
				right_value = prev(left_iter)->second == m_valBegin ? m_valBegin : prev(left_iter)->second;
				if (prev(left_iter)->second == val){
					left_iter--;
				}
			}
			auto right_bound = m_map.emplace(keyEnd,right_value);
			auto right_iter = right_bound.first;
			if(prev(right_iter) != left_iter){
				right_iter->second = prev(right_iter)->second;
				if(right_iter->second == val){
					right_iter++;
				}
				m_map.erase(next(left_iter),right_iter);
			}
			if(next(right_iter) == m_map.end()){
				right_iter->second = m_valBegin;
			}
		}
	}

	V const& operator[]( K const& key ) const {
		auto it=m_map.upper_bound(key);
		if(it==m_map.begin()) {
			return m_valBegin;
		} else {
			return (--it)->second;
		}
	}

	size_t GetSize() const{
		return m_map.size();
	}
	vector<Interval> GetIntervals(const K& left, const K& right) const{
		if(m_map.empty()){
			return {{left,right, m_valBegin}};
		}
		vector<Interval> interval;
		auto begin_int = m_map.upper_bound(left);
		auto end_int = m_map.upper_bound(right);
		if(begin_int != m_map.begin()){
			interval.push_back({left,min(begin_int->first,right),prev(begin_int)->second});
		}
		for(; begin_int!= m_map.end() && next(begin_int) != end_int; begin_int++){
			if(begin_int->second !=  m_valBegin)
				interval.push_back({begin_int->first,next(begin_int)->first,begin_int->second});

		}
		if(begin_int->first < right){
			interval.push_back({begin_int->first,right,begin_int->second});
		}
		return move(interval);
	}

private:
	V m_valBegin;	//  значение по умолчанию
	std::map<K,V> m_map;	// Сам контейнер
};
class PersonalBugetManager{
public:
	PersonalBugetManager() : intervals(0){}

	float ComputeIncome(const Date& from, const Date& to) const{
		cout<<"ComputeIncome"<<endl;
		vector<Intervals<Date,double>::Interval> buffer = intervals.GetIntervals(from,to);
		double sum = 0;
		cout<<buffer.size()<<endl;
		for(const auto& item: buffer){
			cout<<item.start.Print()<<" - "<<item.end.Print()<<endl;
			cout<<(2 + ComputeDaysDiff(item.end, item.start))<<" ";
			cout<<item.value<<"- income"<<endl;
			sum += item.value*(2 + ComputeDaysDiff(item.end, item.start));  // -1
		}
		return sum;
	}

	void Earn(const Date& from, const Date& to, double value){
		cout<<"Earn"<<endl;
		vector<Intervals<Date,double>::Interval> buffer = intervals.GetIntervals(from,to);
		double earned = value / (2 + ComputeDaysDiff(to, from));
		cout<<"earned - "<< earned<<endl;
		cout<<buffer.size()<<endl;
		for(auto& item: buffer){
			cout<<item.start.Print()<<" - "<<item.end.Print()<<endl;
			//cout<<item.value<<"- income"<<endl;
			intervals.assign(item.start, item.end, item.value + earned);
		}
	}

	void PayTax(const Date& from, const Date& to){
		cout<<"PayTax"<<endl;
		vector<Intervals<Date,double>::Interval> buffer = intervals.GetIntervals(from,to);
		cout<<buffer.size()<<endl;
		for(auto& item: buffer){
			cout<<item.start.Print()<<" - "<<item.end.Print()<<endl;
			cout<<(2 + ComputeDaysDiff(item.end, item.start))<<" ";
			cout<<item.value<<"- income"<<endl;
			intervals.assign(item.start, item.end, item.value*TAX);
		}
	}
private:
	Intervals<Date, double> intervals;
	const double TAX = 0.87;
};
void TestDate();
void TestIntervals();
void SimpleTest();
void AdvanceTest();

int main(){
	TestRunner tr;
	RUN_TEST(tr,TestDate);
	RUN_TEST(tr,TestIntervals);
	RUN_TEST(tr,SimpleTest);
	//RUN_TEST(tr,SimpleTest);

	return 0;
}

void TestDate(){
	Date date_from("2019-06-15");
	std::tm ans_from({0,0,0,15,5,119,0});
	Date date_to("2020-07-16");
	std::tm ans_to({0,0,0,16,6,120,0});

	ASSERT_EQUAL(date_from.AsTimestamp(),mktime(&ans_from));
	ASSERT_EQUAL(date_to.AsTimestamp(),mktime(&ans_to));

	ASSERT_EQUAL(ComputeDaysDiff(date_to, date_from), 397);

	ASSERT(date_from < date_to);
	ASSERT(!(date_to < date_from));

	++date_to;
	std::tm ans_to_next({0,0,0,17,6,120,0});
	ASSERT_EQUAL(date_to.AsTimestamp(),mktime(&ans_to_next));

	Date end_of_month("2019-06-30");
	++end_of_month;
	std::tm end_of_month_next({0,0,0,1,6,119,0});
	ASSERT_EQUAL(end_of_month.AsTimestamp(),mktime(&end_of_month_next));

	Date end_of_year("2019-12-31");
	++end_of_year;
	std::tm end_of_year_next({0,0,0,1,0,120,0});
	ASSERT_EQUAL(end_of_year.AsTimestamp(),mktime(&end_of_year_next));
}

void TestOneCaseForIntervals(const Intervals<int,char>& int_map, const string& ans){
	for(size_t i = 0; i < ans.length(); i++){
		ASSERT_EQUAL(int_map[i], ans[i]);
	}
}
void TestIntervals(){
	//Constuctor
	{
		Intervals<int, char> int_map('A');
		string answer = "AAAAAAAAAA";

		TestOneCaseForIntervals(int_map, answer);
	}

	// X_left, X_right D_left, D_right
	{
		Intervals<int, char> int_map('A');
		int_map.assign(2,6,'D');
		int_map.assign(0,1,'X');
		string answer = "XADDDDAAAA";

		TestOneCaseForIntervals(int_map, answer);
	}

	//X_left, X_right D_left, D_right
	{
		Intervals<int, char> int_map('A');
		int_map.assign(2,6,'D');
		int_map.assign(6,8,'X');
		string answer = "AADDDDXXAA";

		TestOneCaseForIntervals(int_map, answer);
	}

	//X_left, X_right D_left, D_right
	{
		Intervals<int, char> int_map('A');
		int_map.assign(2,6,'D');
		int_map.assign(1,3,'X');
		string answer = "AXXDDDAAAA";

		TestOneCaseForIntervals(int_map, answer);
	}
	//X_left, X_right D_left, D_right

	{
		Intervals<int, char> int_map('A');
		int_map.assign(2,6,'D');
		int_map.assign(4,7,'X');
		string answer = "AADDXXXAAA";

		TestOneCaseForIntervals(int_map, answer);
	}
	//X_left, X_right D_left, D_right

	{
		Intervals<int, char> int_map('A');
		int_map.assign(2,6,'D');
		int_map.assign(0,2,'X');
		string answer = "XXDDDDAAAA";

		TestOneCaseForIntervals(int_map, answer);
	}

	//X_left, X_right D_left, D_right
	{
		Intervals<int, char> int_map('A');
		int_map.assign(2,6,'D');
		int_map.assign(5,8,'X');
		string answer = "AADDDXXXAA";

		TestOneCaseForIntervals(int_map, answer);
	}

	//X_left, X_right D_left, D_right
	{
		Intervals<int, char> int_map('A');
		int_map.assign(2,6,'D');
		int_map.assign(3,5,'X');
		string answer = "AADXXDAAAA";

		TestOneCaseForIntervals(int_map, answer);
	}

	//A secuence of assign-method
	Intervals<int, char> int_map('A');
	{
		int_map.assign(1,3,'B');
		int_map.assign(4,6,'C');
		string answer = "ABBACCA";
		TestOneCaseForIntervals(int_map, answer);
	}
	{
		int_map.assign(2,6,'D');
		string answer = "ABDDDDA";
		TestOneCaseForIntervals(int_map, answer);
	}
	//Insert the same value in sub-interval
	{
		int_map.assign(3,5,'D');
		string answer = "ABDDDDA";
		TestOneCaseForIntervals(int_map, answer);
		ASSERT_EQUAL(int_map.GetSize(),3u);

		int_map.assign(3,5,'D');
	}
	{
		Intervals<int,char> new_int_map('A');
		new_int_map.assign(3,7,'D');
		string answer = "AAADDDDA";
		TestOneCaseForIntervals(new_int_map, answer);
		ASSERT_EQUAL(new_int_map.GetSize(),2u);

		int_map.assign(3,5,'D');
	}


}
void SimpleTest(){
	PersonalBugetManager pbm;
	pbm.Earn(Date("2001-01-02"), Date("2001-01-06"), 20);


	ASSERT_EQUAL(pbm.ComputeIncome(Date("2001-01-01"), Date("2002-01-01")),20);
	//pbm.Earn(Date("2001-01-02"), Date("2001-01-06"), 10);
	//ASSERT_EQUAL(pbm.ComputeIncome(Date("2001-01-01"), Date("2001-01-01")),30);
	pbm.PayTax(Date("2001-01-02"), Date("2001-01-03"));

	ASSERT_EQUAL(pbm.ComputeIncome(Date("2001-01-01"), Date("2002-01-01")),18.96);
	/*pbm.Earn(Date("2001-01-03"), Date("2001-01-03"), 10);

	ASSERT_EQUAL(pbm.ComputeIncome(Date("2001-01-01"), Date("2001-01-01")),28.96);
	pbm.PayTax(Date("2001-01-03"), Date("2001-01-03"));

	ASSERT_EQUAL(pbm.ComputeIncome(Date("2001-01-01"), Date("2001-01-01")),27076);*/
}

void AdvanceTest(){

}
