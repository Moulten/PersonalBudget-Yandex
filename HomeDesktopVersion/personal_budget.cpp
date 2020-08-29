#include "test_runner.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <map>
#include <set>

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
	Date& operator++(){
		day_++;
		if(day_ > days_in_month[month_]){
			day_ = 1;
			month_++;
		}
		if(month_ > 12){
			month_ = 1;
			year_++;
		}
		return *this;
	}

	Date& operator--(){
		day_--;
		if(day_ == 0){
			month_--;
			if(!month_ ){
				month_ = 12;
				year_--;
			}
			day_ = days_in_month[month_];
		}
		return *this;
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
class SetIntervals {
public:
	struct Interval{
		K start;
		K end;
		V value;

		Interval(const K& start, const K& end, V value) :
			start(start), end(end), value(value){}

	/*	bool operator<(const Interval& rhs) const {
		      return start < rhs.start;
		}*/
	};

private:
	  struct Compare
	  {
	    using is_transparent = void;

	    bool operator()(const Interval& lhs, const Interval& rhs) const {
	      return lhs.start < rhs.start;
	    }
	    bool operator()(const K& lhs, const Interval& rhs) const {
	      return lhs < rhs.start;
	    }
	    bool operator()(const Interval& lhs, const K& rhs) const {
	      return lhs.start < rhs;
	    }
	  };

public:
	SetIntervals(V const& val)
	: m_valBegin(val)
	{}

	void assign(const K& keyBegin, const K& keyEnd, V const& val ) {
		if(keyBegin < keyEnd){
			auto left_empl = i_set.emplace(Interval(keyBegin,keyEnd, val));
			auto left_iter = left_empl.first;
			// If keyBegin already exist
			if(!left_empl.second){
				auto old_value = i_set.extract(left_iter).value();

				if(keyEnd < old_value.end){
					K key_end = keyEnd;
					i_set.insert(Interval(++key_end,old_value.end,old_value.value));
				}
				i_set.insert({keyBegin, keyEnd, val});
			}
			//Insertion into an exit section
			else if(left_iter != i_set.begin() && keyBegin < prev(left_iter)->end){
				Interval left_sec = i_set.extract(prev(left_iter)).value();
				if(keyEnd < left_sec.end){
					K key_end = keyEnd;
					i_set.insert(Interval(++key_end,left_sec.end,left_sec.value));
				}
				K key_start = keyBegin;
				i_set.insert(Interval(left_sec.start,--key_start,left_sec.value));
			}
			if(auto right_iter = i_set.upper_bound(keyEnd);
				right_iter != i_set.begin() && keyEnd < prev(right_iter)->end){
				//i_set.erase(left_iter,prev(right_iter));
				Interval right_sec = i_set.extract(prev(right_iter)).value();
				K key_end = keyEnd;
				i_set.insert(Interval(++key_end,right_sec.end,right_sec.value));
			}
		}

	}

	size_t GetSize() const{
		return i_set.size();
	}
	vector<Interval> GetIntervals(const K& left, const K& right) const{
		if(i_set.empty()){
			return {Interval(left, right, m_valBegin)};
		}
		vector<Interval> interval;
		auto left_iter = i_set.upper_bound(left);
		if(left_iter != i_set.begin() && left < prev(left_iter)->end){
			interval.push_back({left,min(prev(left_iter)->end, right),prev(left_iter)->value});
		}
		for(; left_iter != i_set.end() && left_iter->start < right; left_iter++){
			interval.push_back({left_iter->start, min(left_iter->end, right),left_iter->value});
		}
		/*if(left_iter != i_set.end() && right < left_iter->end){
			interval.push_back({left_iter->start, right,left_iter->value});
		}*/
		return move(interval);
	}

private:
	V m_valBegin;	//  значение по умолчанию
	std::set<Interval,Compare> i_set;	// Сам контейнер
};
class PersonalBugetManager{
public:
	PersonalBugetManager() : intervals(0){}

	double ComputeIncome(const Date& from, const Date& to) const{
		cout<<"ComputeIncome"<<endl;
		vector<SetIntervals<Date,double>::Interval> buffer = intervals.GetIntervals(from,to);
		double sum = 0;
		for(const auto& item: buffer){
			sum += item.value*(1 + ComputeDaysDiff(item.end, item.start));  // -1
		}
		return sum;
	}

	void Earn(const Date& from, const Date& to, double value){
		vector<SetIntervals<Date,double>::Interval> buffer = intervals.GetIntervals(from,to);
		double earned = value / (1 + ComputeDaysDiff(to, from));
		for(auto& item: buffer){
			intervals.assign(item.start, item.end, item.value + earned);
		}
	}

	void PayTax(const Date& from, const Date& to){
		vector<SetIntervals<Date,double>::Interval> buffer = intervals.GetIntervals(from,to);
		for(auto& item: buffer){
			intervals.assign(item.start, item.end, item.value*TAX);
		}
	}
private:
	SetIntervals<Date, double> intervals;
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

	ASSERT_EQUAL(ComputeDaysDiff(Date("2010-01-06"), Date("2010-01-02")), 4);
}

void CheckIntervalsInt(const vector<SetIntervals<int,char>::Interval>& buffer,
		const vector<pair<int,char>>& answer){
	for(size_t i = 0; i < buffer.size(); i++){
		ASSERT_EQUAL((buffer[i].end - buffer[i].start), answer[i].first);
		ASSERT_EQUAL(buffer[i].value, answer[i].second);
	}
}

void TestIntervals(){
	SetIntervals<int,char> i_set('A');
	i_set.assign(2,6,'B');
	i_set.assign(7,9,'C');
	i_set.assign(1,1,'G');
	vector<SetIntervals<int,char>::Interval> buffer = i_set.GetIntervals(0,10);
	ASSERT_EQUAL(buffer.size(),3u);

	//check insertion in the midle of interval
	i_set.assign(3,4,'X');
	buffer = i_set.GetIntervals(0,10);
	ASSERT_EQUAL(buffer.size(),5u);
	vector<pair<int,char>> answer = {
			{0,'G'}, {0,'B'}, {1,'X'}, {1,'B'}, {2,'C'},
	};
	CheckIntervalsInt(buffer, answer);

	//check insertion of big interval
	i_set.assign(0,10, 'P');
	buffer = i_set.GetIntervals(0,10);
	ASSERT_EQUAL(buffer.size(),1u);

	//check right assign
	i_set.assign(8,12,'B');
	answer = {{7,'P'}, {4,'B'}};
	buffer = i_set.GetIntervals(0,15);
	CheckIntervalsInt(buffer, answer);

	//check left assign
	i_set.assign(-4,3,'B');
	answer = {{7,'J'},{3,'P'}, {4,'B'}};
	buffer = i_set.GetIntervals(-7,15);
	CheckIntervalsInt(buffer, answer);

}
void SimpleTest(){
	PersonalBugetManager pbm;
	pbm.Earn(Date("2001-01-02"), Date("2001-01-06"), 20);
	ASSERT_EQUAL(pbm.ComputeIncome(Date("2001-01-01"), Date("2002-01-01")),20);

	pbm.PayTax(Date("2001-01-02"), Date("2001-01-03"));
	ASSERT_EQUAL(pbm.ComputeIncome(Date("2001-01-01"), Date("2002-01-01")),18.96);

	pbm.Earn(Date("2001-01-03"), Date("2001-01-03"), 10);
	ASSERT_EQUAL(pbm.ComputeIncome(Date("2001-01-01"), Date("2002-01-01")),28.96);

	pbm.PayTax(Date("2001-01-03"), Date("2001-01-03"));
	ASSERT_EQUAL(pbm.ComputeIncome(Date("2001-01-01"), Date("2002-01-01")),27076);
}

