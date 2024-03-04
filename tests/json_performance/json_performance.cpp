#include "glaze/glaze.hpp"
#include "glaze/glaze_exceptions.hpp"
#include "glaze/core/macros.hpp"
#include "glaze/api/impl.hpp"
#include "glaze/qmap.hpp"
#include "glaze/qstring.hpp"

#include <memory>
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <map>

#include <cxxabi.h>

#ifdef HAVE_QT
   #include <QObject>
   #include <QVector>
   //#include <QMap>
   #include <QSet>
   #include <QList>
#endif

static constexpr int objectListSize = 300000;

static constexpr std::string_view json0 = R"(
{
   "fixed_object": {
      "int_array": [0, 1, 2, 3, 4, 5, 6],
      "float_array": [0.1, 0.2, 0.3, 0.4, 0.5, 0.6],
      "double_array": [3288398.238, 233e22, 289e-1, 0.928759872, 0.22222848, 0.1, 0.2, 0.3, 0.4]
   },
   "fixed_name_object": {
      "name0": "James",
      "name1": "Abraham",
      "name2": "Susan",
      "name3": "Frank",
      "name4": "Alicia"
   },
   "another_object": {
      "string": "here is some text",
      "another_string": "Hello World",
      "escaped_text": "{\"some key\":\"some string value\"}",
      "boolean": false,
      "nested_object": {
         "v3s": [[0.12345, 0.23456, 0.001345],
                  [0.3894675, 97.39827, 297.92387],
                  [18.18, 87.289, 2988.298]],
         "id": "298728949872"
      }
   },
   "string_array": ["Cat", "Dog", "Elephant", "Tiger"],
   "string": "Hello world",
   "number": 3.14,
   "boolean": true,
   "another_bool": false
}
)";

struct fixed_object_t
{
   std::vector<int> int_array{};
   std::vector<float> float_array{};
   std::vector<double> double_array{};
};

struct fixed_name_object_t
{
   std::string name0{"wqregdfsdf"};
   std::string name1{"dsfghjpoiuztrklsjdhfpőeokan"};
   std::string name2{"124512"};
   std::string name3{"ffffffffff"};
   std::string name4{"adhjkláé"};
/*
   template<class Archive> 
   void serialize(Archive & ar) { 
      ar( CEREAL_NVP(name0), CEREAL_NVP(name1), CEREAL_NVP(name2), CEREAL_NVP(name3), CEREAL_NVP(name4) ); 
   } 
   */
};

struct nested_object_t
{
   std::vector<std::array<double, 3>> v3s{};
   std::string id{"123123"};
/*
   template<class Archive> 
   void serialize(Archive & ar) { 
      ar( CEREAL_NVP(v3s), CEREAL_NVP(id)); 
   }
   */
};

struct another_object_t
{
   std::string string;
   std::string another_string;
   std::string escaped_text;
   bool boolean{false};
   nested_object_t nested_object{};
/*
   template<class Archive> 
   void serialize(Archive & ar) { 
      ar( CEREAL_NVP(string), CEREAL_NVP(another_string), CEREAL_NVP(escaped_text), CEREAL_NVP(boolean), CEREAL_NVP(nested_object) ); 
   } 
   */
};

struct another_object_variant1_t : public another_object_t
{
   another_object_variant1_t() {
      string = "qweqwe";
      another_string = "qweqweqwe";
      escaped_text = "qweqweqwe";
      boolean = false;
   };
};

struct another_object_variant2_t : public another_object_t

{
   another_object_variant2_t() {
      string = "asdasd";
      another_string = "asdasdasd";
      escaped_text = "asdasdasd";
      boolean = true;
   };
};

struct obj_t : QObject
{
   friend class glz::meta<obj_t>;
   //fixed_object_t fixed_object{};
   //std::shared_ptr<fixed_name_object_t> fixed_name_object;
   QMap<int, another_object_t> qmap{};
   //const std::string string{};
   //QString qString{};
   //double number{};
   //bool boolean{};
   //bool another_bool{};

public:
   obj_t() = default;
   void init (int i) {
      
      //fixed_name_object = std::make_shared<fixed_name_object_t>();
      for(int j = 0; j < i; ++j) {
         qmap.insert(j,another_object_variant1_t{});
      }

      for(int j = 0; j < i; ++j) {
         qmap.insert((i + j), another_object_variant2_t{});
      }
      /*
      for(int i = 0; i < objectListSize; ++i) {
         qString.push_back("This is a QString plea\"se work\nyeeeah"); 
      } 

      */
   }
/*
   void printQMap() {
      std::cout << "QMap: ";
      for(auto p : qmap.keys()) {
         std::cout << p;
      }
      std::cout << std::endl;
   }

   QMap<int, another_object_t>& map() {
      return qmap;
   }
*/

   template<class obj_t, class Buffer>
   friend auto write_binary(obj_t&&, Buffer&&);

   template<class obj_t, class Buffer>
   friend glz::parse_error read_binary(obj_t&&, Buffer&&);

   template<class obj_t, class Buffer>
   friend auto write_json(obj_t&&, Buffer&&);

   template<class obj_t, class Buffer>
   friend glz::parse_error read_json(obj_t&&, Buffer&&);
};


template<>
struct glz::meta<obj_t> {
   using T = obj_t;
   static constexpr auto value = object(
      //&T::fixed_object,
      //&T::fixed_name_object,
      &T::qmap
      //&T::string,
      //&T::qString,
      //&T::number,
      //&T::boolean,
      //&T::another_bool
   );
};

struct obj_t_std
{
   friend class glz::meta<obj_t_std>;
   //fixed_object_t fixed_object{};
   //std::shared_ptr<fixed_name_object_t> fixed_name_object;
   std::map<int, another_object_t> qmap{};
   //const std::string string{};
   //std::string qString{};
   //double number{};
   //bool boolean{};
   //bool another_bool{};

public:
   obj_t_std() = default;
   void init (int i) {
      
      //fixed_name_object = std::make_shared<fixed_name_object_t>();
      for(int j = 0; j < i; ++j) {
         qmap.insert({j,another_object_variant1_t{}});
      }

      for(int j = 0; j < i; ++j) {
         qmap.insert({(i + j), another_object_variant2_t{}});
      }
      /*
      for(int i = 0; i < objectListSize; ++i) {
         qString.append("This is a QString plea\"se work\nyeeeah"); 
      } 
      */
   }
/*
   void printQMap() {
      std::cout << "QMap: ";
      for(auto p : qmap.keys()) {
         std::cout << p;
      }
      std::cout << std::endl;
   }
   QMap<int, another_object_t>& map() {
      return qmap;
   }

*/


   template<class obj_t_std, class Buffer>
   friend auto write_binary(obj_t_std&&, Buffer&&);

   template<class obj_t_std, class Buffer>
   friend glz::parse_error read_binary(obj_t_std&&, Buffer&&);

   template<class obj_t_std, class Buffer>
   friend auto write_json(obj_t_std&&, Buffer&&);

   template<class obj_t_std, class Buffer>
   friend glz::parse_error read_json(obj_t_std&&, Buffer&&);
};


template<>
struct glz::meta<obj_t_std> {
   using T = obj_t_std;
   static constexpr auto value = object(
      //&T::fixed_object,
      //&T::fixed_name_object,
      &T::qmap
      //&T::string,
      //&T::qString,
      //&T::number,
      //&T::boolean,
      //&T::another_bool
   );
};

template <>
struct glz::meta<fixed_object_t> {
   using T = fixed_object_t;
   static constexpr auto value = object(
      &T::int_array,
      &T::float_array,
      &T::double_array
   );
};

template <>
struct glz::meta<nested_object_t> {
   using T = nested_object_t;
   static constexpr auto value = object(
      &T::v3s,
      &T::id
   );
};

template <>
struct glz::meta<another_object_t> {
   using T = another_object_t;
   static constexpr auto value = object(
      &T::string,
      &T::another_string,
      &T::escaped_text,
      &T::boolean,
      &T::nested_object
   );
};

/*
template <>
struct glz::meta<obj_t<another_object_variant1_t>> {
   using T = obj_t<another_object_variant1_t>;
   static constexpr auto value = object(
      &T::fixed_object,
      &T::fixed_name_object,
      &T::string_array,
      &T::string,
      &T::number,
      &T::boolean,
      &T::another_bool
   );
};

template <>
struct glz::meta<obj_t<another_object_variant2_t>> {
   using T = obj_t<another_object_variant2_t>;
   static constexpr auto value = object(
      &T::fixed_object,
      &T::fixed_name_object,
      &T::string_array,
      &T::string,
      &T::number,
      &T::boolean,
      &T::another_bool
   );
};
*/

/*
template <>
struct glz::meta<obj_t> {
   using T = obj_t;
   static constexpr auto value = object(
      &T::fixed_object,
      &T::fixed_name_object,
      &T::string_array,
      &T::string,
      &T::number,
      &T::boolean,
      &T::another_bool
   );
};
*/

#ifdef NDEBUG
static constexpr size_t iterations = 1'000'000;
static constexpr size_t iterations_abc = 10'000;
#else
static constexpr size_t iterations = 100'000;
static constexpr size_t iterations_abc = 1'000;
#endif

// We scale all speeds by the minified JSON byte length, so that libraries which do not efficiently write JSON do not get an unfair advantage
// We want to know how fast the libraries will serialize/deserialize with repsect to one another
size_t minified_byte_length{};

struct results
{
   std::string_view name{};
   std::string_view url{};
   size_t iterations{};
   
   std::optional<size_t> json_byte_length{};
   std::optional<double> json_read{};
   std::optional<double> json_write{};
   std::optional<double> json_roundtrip{};
   
   std::optional<size_t> binary_byte_length{};
   std::optional<double> binary_write{};
   std::optional<double> binary_read{};
   std::optional<double> binary_roundtrip{};
   
   void print(bool use_minified = true)
   {
      if (json_roundtrip) {
         std::cout << name << " json roundtrip: " << *json_roundtrip << " s\n";
      }
      
      if (json_byte_length) {
         std::cout << name << " json byte length: " << *json_byte_length << '\n';
      }
      
      if (json_write) {
         if (json_byte_length) {
            const auto byte_length = use_minified ? minified_byte_length : *json_byte_length;
            const auto MBs = iterations * byte_length / (*json_write * 1048576);
            std::cout << name << " json write: " << *json_write << " s, " << MBs << " MB/s\n";
         }
         else {
            std::cout << name << " json write: " << *json_write << " s\n";
         }
      }
      
      if (json_read) {
         if (json_byte_length) {
            const auto byte_length = use_minified ? minified_byte_length : *json_byte_length;
            const auto MBs = iterations * byte_length / (*json_read * 1048576);
            std::cout << name << " json read: " << *json_read << " s, " << MBs << " MB/s\n";
         }
         else {
            std::cout << name << " json read: " << *json_read << " s\n";
         }
      }
      
      if (binary_roundtrip) {
         std::cout << '\n';
         std::cout << name << " binary roundtrip: " << *binary_roundtrip << " s\n";
      }
      
      if (binary_byte_length) {
         std::cout << name << " binary byte length: " << *binary_byte_length << '\n';
      }
      
      if (binary_write) {
         if (binary_byte_length) {
            const auto MBs = iterations * *binary_byte_length / (*binary_write * 1048576);
            std::cout << name << " binary write: " << *binary_write << " s, " << MBs << " MB/s\n";
         }
         else {
            std::cout << name << " binary write: " << *binary_write << " s\n";
         }
      }
      
      if (binary_read) {
         if (binary_byte_length) {
            const auto MBs = iterations * *binary_byte_length / (*binary_read * 1048576);
            std::cout << name << " binary read: " << *binary_read << " s, " << MBs << " MB/s\n";
         }
         else {
            std::cout << name << " binary read: " << *binary_read << " s\n";
         }
      }
      
      std::cout << "\n---\n" << std::endl;
   }
};

/*
template <class T>
inline bool is_valid_write(const std::string& buffer, const std::string& library_name) {
   T obj{};
   
   glz::ex::read_json(obj, json0);
   
   std::string reference{}; // reference to compare again
   glz::write_json(obj, reference);
   
   obj = {};
   glz::ex::read_json(obj, buffer);
   
   std::string compare{};
   glz::write_json(obj, compare);
   
   if (reference != compare) {
      std::cout << "Invalid write for library: " << library_name << std::endl;
      return false;
   }
   
   return true;
}
*/

static obj_t obj{};
static obj_t_std obj_std{};

auto glaze_test_qt_binary()
{
   std::string buffer{};
   obj_t other{};

   auto t0 = std::chrono::steady_clock::now();

   glz::write_binary(obj, buffer);
   auto error = glz::read_binary(other, buffer);

   //std::cout << "glaze error: " << error << std::endl;
   
   auto t1 = std::chrono::steady_clock::now();
   
   results r{ "Glaze Qt binary", "https://github.com/stephenberry/glaze", iterations };
   r.json_roundtrip = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() * 1e-6;

   std::cout << "buffer size: " << buffer.size() <<std::endl;
   //std::cout << "string" <<other.qString.toStdString() << std::endl;
   return r;
}

auto glaze_test_qt_json()
{
   std::string buffer{};
   obj_t other{};

   auto t0 = std::chrono::steady_clock::now();

   glz::write_json(obj, buffer);
   auto error = glz::read_json(other, buffer);

   //std::cout << "glaze error: " << error << std::endl;
   
   auto t1 = std::chrono::steady_clock::now();
   
   results r{ "Glaze Qt json", "https://github.com/stephenberry/glaze", iterations };
   r.json_roundtrip = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() * 1e-6;

   std::cout << "buffer size: " << buffer.size() <<std::endl;
   //std::cout << "string" <<other.qString.toStdString() << std::endl;
   return r;
}

auto glaze_test_std_binary()
{
   std::string buffer{};

   obj_t_std other{};
   auto t0 = std::chrono::steady_clock::now();

   glz::write_binary(obj_std, buffer);
   auto error = glz::read_binary(other, buffer);
   
   //std::cout << "glaze error: " << error << std::endl;
   
   auto t1 = std::chrono::steady_clock::now();
   
   results r{ "Glaze std binary", "https://github.com/stephenberry/glaze", iterations };
   r.json_roundtrip = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() * 1e-6;

   //std::cout << buffer <<std::endl;
   //std::cout <<"string: " << other.qString << std::endl;
   return r;
}

auto glaze_test_std_json()
{
   std::string buffer{};

   obj_t_std other{};
   auto t0 = std::chrono::steady_clock::now();

   glz::write_json(obj_std, buffer);
   auto error = glz::read_json(other, buffer);
   
   //std::cout << "glaze error: " << error << std::endl;
   
   auto t1 = std::chrono::steady_clock::now();
   
   results r{ "Glaze std json", "https://github.com/stephenberry/glaze", iterations };
   r.json_roundtrip = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() * 1e-6;

   //std::cout << buffer <<std::endl;
   //std::cout <<"string: " << other.qString << std::endl;
   return r;
}


void test0()
{
   std::vector<results> results;
   //for(int i = 0; i < 10; ++i)
      results.emplace_back(glaze_test_qt_binary());
   
   //for(int i = 0; i < 10; ++i)
      results.emplace_back(glaze_test_qt_json());

   //for(int i = 0; i < 10; ++i)
      results.emplace_back(glaze_test_std_binary());

   //for(int i = 0; i < 10; ++i)
      results.emplace_back(glaze_test_std_json());

   for (auto r : results) {
      r.print();
   }
}

int main()
{
   obj.init(objectListSize);
   obj_std.init(objectListSize);
   test0();
   
   return 0;
}