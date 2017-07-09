/* Performance measurements of traversal through a levelorder_vector.
 *
 * Copyright 2015 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */
    
#include <algorithm>
#include <array>
#include <chrono>
#include <numeric> 
  
std::chrono::high_resolution_clock::time_point measure_start,measure_pause;
      
template<typename F>
double measure(F f)
{
  using namespace std::chrono;
      
  static const int              num_trials=10;
  static const milliseconds     min_time_per_trial(200);
  std::array<double,num_trials> trials;
  static decltype(f())          res; /* to avoid optimizing f() away */
      
  for(int i=0;i<num_trials;++i){
    int                               runs=0;
    high_resolution_clock::time_point t2;
      
    measure_start=high_resolution_clock::now();
    do{
      res=f();
      ++runs;
      t2=high_resolution_clock::now();
    }while(t2-measure_start<min_time_per_trial);
    trials[i]=duration_cast<duration<double>>(t2-measure_start).count()/runs;
  }
  (void)res; /* var not used warn */
      
  std::sort(trials.begin(),trials.end());
  return std::accumulate(
    trials.begin()+2,trials.end()-2,0.0)/(trials.size()-4);
}
      
void pause_timing()
{
  measure_pause=std::chrono::high_resolution_clock::now();
}
      
void resume_timing()
{
  measure_start+=std::chrono::high_resolution_clock::now()-measure_pause;
}
 
#include <algorithm>
#include <vector>
#include <iostream>
#include <boost/iterator/iterator_facade.hpp>

template<typename T>
class levelorder_vector
{
  typedef std::vector<T> vector;
   
public:
  typedef typename vector::value_type             value_type;
  typedef typename vector::reference              reference;
  typedef typename vector::const_reference        const_reference;
  typedef typename vector::difference_type        difference_type;
  typedef typename vector::size_type              size_type;

  class const_iterator:
    public boost::iterator_facade<
      const_iterator,const value_type,boost::forward_traversal_tag
    >
  {
  public:
    const_iterator(const value_type* p=0,size_type i=0,size_type n=0):
      p(p),i(i),n(n){}

  private:
    friend class boost::iterator_core_access;

    bool equal(const const_iterator& x)const{return p==x.p&&i==x.i;}
    const value_type& dereference()const{return p[i];}

    void increment()
    {
      size_type j=right(i);
      if(j<n){
        do{
          i=j;
          j=left(i);
        }while(j<n);
      }
      else if(is_last_leaf(i)){
        i=n;
      }
      else{
        while(is_right_child(i))i=parent(i);
        i=parent(i);
      }
    }

    static size_type parent(size_type j){return (j-1)/2;}
    static size_type right(size_type j){return 2*j+2;}
    static size_type left(size_type j){return 2*j+1;}

    static bool is_last_leaf(size_type j){return is_power_of_two(j+2);}
    static bool is_right_child(size_type j){return j%2==0;}
    static bool is_power_of_two(/* non-null */ size_type j){return !(j&(j-1));}

    const value_type* p;
    size_type         i,n;
  };

  typedef const_iterator iterator;
   
  levelorder_vector(){}
  levelorder_vector(const levelorder_vector& x):impl(x.impl){}
  levelorder_vector& operator=(const levelorder_vector& x){impl=x.impl;return *this;}
   
  template<typename InputIterator>
  levelorder_vector(InputIterator first,InputIterator last)
  {
    vector aux(first,last);
    std::sort(aux.begin(),aux.end());
    impl=aux;
    insert(0,aux.size(),aux.begin());
  }
   
  const_iterator begin()const{return const_iterator(impl.data(),begin_index(),impl.size());}
  const_iterator end()const{return const_iterator(impl.data(),impl.size(),impl.size());}
  const_iterator cbegin()const{return this->begin();}
  const_iterator cend()const{return this->cend();}

  friend bool    operator==(const levelorder_vector& x,const levelorder_vector& y)
                   {return x.impl==y.impl;}
  friend bool    operator!=(const levelorder_vector& x,const levelorder_vector& y)
                   {return x.impl!=y.impl;}
  void           swap(levelorder_vector& x){impl.swap(x.impl);}
  friend void    swap(levelorder_vector& x,levelorder_vector& y){x.swap(y);}
  size_type      size()const{return impl.size();}
  size_type      max_size()const{return impl.max_size();}
  bool           empty()const{return impl.empty();}
   
  const_iterator lower_bound(const T& x)const
  {
    size_type n=impl.size(),i=n,j=0;
    while(j<n){
      if(impl[j]<x){
        j=2*j+2;
      }
      else{
        i=j;
        j=2*j+1;
      }
    }
    return const_iterator(impl.data(),i,impl.size());
  }
   
private:
  void insert(size_type i,size_type n,typename vector::const_iterator first)
  {
    if(n){
      size_type h=root(n);
      impl[i]=*(first+h);
      insert(2*i+1,h,first);
      insert(2*i+2,n-h-1,first+h+1);
    }
  }
 
  size_type root(size_type n)
  {
    if(n<=1)return 0;
    size_type i=2;
    while(i<=n)i*=2;
    return std::min(i/2-1,n-i/4);
  }
   
  size_type begin_index()const
  {
    size_type a=1,n=impl.size();
    while(a<n)a*=2;
    return a/2-1;
  }

  vector impl;
};
 
#include <iostream>
#include <functional>
#include <vector>
 
template<typename Container>
Container create(unsigned int n)
{
  std::vector<unsigned int> v;
  for(unsigned int m=0;m<n;++m)v.push_back(m);
  return Container(v.begin(),v.end());
}
 
template<typename Container>
struct traverse
{
  typedef unsigned int result_type;
  
  unsigned int operator()(const Container & c)const
  {
    unsigned int res=0;
    for(const auto& x:c)res+=x;
    return res;
  }
};

template<
  template<typename> class Tester,
  typename Container1,typename Container2,typename Container3>
void test(
  const char* title,
  const char* name1,const char* name2,const char* name3)
{
  unsigned int n0=10000,n1=3000000,dn=2000;
  double       fdn=1.2;
  
  std::cout<<title<<":"<<std::endl;
  std::cout<<name1<<";"<<name2<<";"<<name3<<std::endl;
  
  for(unsigned int n=n0;n<=n1;n+=dn,dn=(unsigned int)(dn*fdn)){
    double t;
 
    {
      auto c=create<Container1>(n);
      t=measure(std::bind(Tester<Container1>(),std::cref(c)));
      std::cout<<n<<";"<<(t/n)*10E6;
    }
    {
      auto c=create<Container2>(n);
      t=measure(std::bind(Tester<Container2>(),std::cref(c)));
      std::cout<<";"<<(t/n)*10E6;
    }
    {
      auto c=create<Container3>(n);
      t=measure(std::bind(Tester<Container3>(),std::cref(c)));
      std::cout<<";"<<(t/n)*10E6<<std::endl;
    }
  }
}
 
#include <set>
#include <boost/container/flat_set.hpp>
 
int main()
{
  typedef std::set<unsigned int>                   container_t1;
  typedef boost::container::flat_set<unsigned int> container_t2;
  typedef levelorder_vector<unsigned int>          container_t3;
  
  test<
    traverse,
    container_t1,
    container_t2,
    container_t3>
  (
    "Binary search",
    "std::set",
    "boost::container::flat_set",
    "levelorder_vector"
  );
}
