#include <iostream>
#include <set>
#include <string>

using namespace std;
 
class SetStringer {
 public:
  void insert(double x) { m_set.insert(x); m_cached = false; }
   
  const string & str() & {
    if (m_cached) { return m_string; }
     
    m_string.clear();
     
    for (const auto & x : m_set) {
      m_string += to_string(x) + "\n";
    }
    m_cached = true;
    return m_string;
  }  
 
  string && str() && {
    this->str();
    m_cached = false;
    m_set.clear();
    return move(m_string);
  }
    
 private:
  set<double> m_set;
  string m_string;
  bool m_cached = false;
};

int main()
{
    SetStringer foo;

    // ... add stuff to foo 
    foo.insert(3.14);
    foo.insert(2.36);

    std::cout << foo.str() << std::endl << "============" << std::endl;

    string s = move(foo).str(); // Stealing succeeds!
    cerr << foo.str() << "============" << std::endl; // this probably prints empty

    foo.insert(5.0);
    cerr << foo.str(); // err, what?
    
    return 0;
}
