#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <math.h>
#include <limits>
#include <iomanip>
#include <set>

# define MY_PI 3.14159265358979323846
using namespace std;

class point {
public:
  double x;
  double y;
  point(double X = 0, double Y = 0):x(X), y(Y){}
};

class line {
public:
  point start;  // started point
  point end;    // ended point
  double a;     // represent line as ax + by = c  
  double b;
  double c;

  line(point s, point e, double A, double B, double C) {
    start = s;
    end = e;
    a = A;
    b = B;
    c = C;
  }

  void show_line() {
    cout << "Line: " << endl;
    cout << "(" << start.x << ", " << start.y << "), (" << end.x << ", " << end.y << ")" << endl;
    cout << a << " * x + " << b << " * y = " << c << endl;
    cout << endl;
  }
};

class arc {
public:
  point start;
  point end;
  point c;    // center of cycle
  bool d;     // 0: clockwise, 1: counterclockwise
  double radius;

  arc(point s, point e, point C, bool cw) {
    start = s;
    end = e;
    c = C;
    d = cw;
    radius = get_radius(s, c);
  }

  double get_radius(point s, point c) {
    double x_diff = s.x - c.x;
    double y_diff = s.y - c.y;
    return sqrt(x_diff * x_diff + y_diff * y_diff);
  }

  void show_arc() {
    cout << "Arc: " << endl;
    cout << "(" << start.x << ", " << start.y << "), (" << end.x << ", " << end.y << ")" << endl;
    cout << "Center: (" << c.x << ", " << c.y << "), Radius: " << radius << ", ";
    if (!d) cout << "CW" << endl;
    else cout << "CCW" << endl;
    cout << endl;
  }
}; 

class polygon {
public:
  int type; // 0: assembly, 1: copper, 2: silkscreen
  double x_max;
  double x_min;
  double y_max;
  double y_min;
  double r_max;
  vector<line> line_set;
  vector<arc>  arc_set;

  polygon(int t = 0): type(t) {}

  void clear(int t) {
    type = t; // 0: assembly, 1: copper, 2: silkscreen
    x_max = 0;
    x_min = 0;
    y_max = 0;
    y_min = 0;
    r_max = 0;
    line_set.clear();
    arc_set.clear();
  }
};

class graph {
private:
  double x_max;
  double x_min;
  double y_max;
  double y_min;
  double r_max;
  double assemblygap;
  double coppergap;
  double silkscreenlen;
  vector<polygon> init_list;
  vector<polygon> extended_list;
public:
  void initialize(ifstream &in_file);
  void extend_polygon();
  void find_max();
  void show_polygons();
  void show_line_set(polygon p);
  void show_arc_set(polygon p);
  void show_result(ofstream &out_file);
  polygon extend(const polygon p);
  point GetCrossPoint(line l1, line l2);
  vector<string> split(const string& str, const string& pattern);
};

int main(int argc, char* argv[])
{
  // declare in-out format
	ifstream in_file;
	ofstream out_file;
  string s;
  graph g;
  int n = 0;

  in_file.open(argv[1]);    //open <.in file>
	if(!in_file.good()) cout << "Open failed!!!" << endl;

  g.initialize(in_file);
  g.find_max();
  // g.extend_polygon();
  // g.show_polygons();

  out_file.open(argv[2]);    //open <.out file>
	if(!out_file.good()) cout << "Open failed!!!" << endl;
  g.show_result(out_file);

  return 0;
}

void graph::initialize(ifstream &in_file) {
  string str;
  string pattern = ",";
  vector<string> str_list;

  // get assembly gap
  getline(in_file, str, '\n');
  str_list = split(str, pattern);
  assemblygap = stof(str_list[1]);
  str_list.clear();
  // get copper gap
  getline(in_file, str, '\n');
  str_list = split(str, pattern);
  coppergap = stof(str_list[1]);
  str_list.clear();
  // get minimum length of silkscreen
  getline(in_file, str, '\n');
  str_list = split(str, pattern);
  silkscreenlen = stof(str_list[1]);
  str_list.clear();

  // init
  getline(in_file, str, '\n');
  polygon P0(0);
  while (getline(in_file, str, '\n')) {
    if (str == "copper") {
      cout << "Break!" << endl;
      break;
    }
    str_list = split(str, pattern);

    if (str_list[0] == "line") {
      double p1_x = stof(str_list[1]);
      double p1_y = stof(str_list[2]);
      double p2_x = stof(str_list[3]);
      double p2_y = stof(str_list[4]);
      double A = p2_y - p1_y;
      double B = p1_x - p2_x;
      double C = p1_x * p2_y - p2_x * p1_y;
      point p1(p1_x, p1_y);
      point p2(p2_x, p2_y);
      if (A < 0 || (A == 0 && B < 0)) {
        A *= -1; B *= -1; C *= -1;
      }
      if (A != 0) {
        B = B / A; C = C / A; A = 1;
      } else {
        A = A / B; C = C / B; B = 1;
      }
      P0.line_set.push_back(line(p1, p2, A, B, C));
    } else if (str_list[0] == "arc") {
      double p1_x = stof(str_list.at(1));
      double p1_y = stof(str_list.at(2));
      double p2_x = stof(str_list.at(3));
      double p2_y = stof(str_list.at(4));
      double pc_x = stof(str_list.at(5));
      double pc_y = stof(str_list.at(6));
      double A = p2_y - p1_y;
      double B = p1_x - p2_x;
      double C = p1_x * p2_y - p2_x * p1_y;
      bool cw = 0;
      point p1(p1_x, p1_y);
      point p2(p2_x, p2_y);
      point pc(pc_x, pc_y);
      if (A < 0 || (A == 0 && B < 0)) {
        A *= -1; B *= -1; C *= -1;
      }
      if (A != 0) {
        B = B / A; C = C / A; A = 1;
      } else {
        A = A / B; C = C / B; B = 1;
      }
      if (str_list.at(7) == "CW") cw = 0;
      else cw = 1;
      P0.line_set.push_back(line(p1, p2, A, B, C));
      P0.arc_set.push_back(arc(p1, p2, pc, cw));
    }
  }
  init_list.push_back(P0);
  // show_line_set(init_list[0]);

  polygon P_copper(1);
  while (getline(in_file, str, '\n')) {
    if (str == "copper") {
      init_list.push_back(P_copper);
      P_copper.clear(1);
    } else {
      str_list = split(str, pattern);

      if (str_list[0] == "line") {
        double p1_x = stof(str_list[1]);
        double p1_y = stof(str_list[2]);
        double p2_x = stof(str_list[3]);
        double p2_y = stof(str_list[4]);
        double A = p2_y - p1_y;
        double B = p1_x - p2_x;
        double C = p1_x * p2_y - p2_x * p1_y;
        point p1(p1_x, p1_y);
        point p2(p2_x, p2_y);
        if (A < 0 || (A == 0 && B < 0)) {
          A *= -1; B *= -1; C *= -1;
        }
        if (A != 0) {
          B = B / A; C = C / A; A = 1;
        } else {
          A = A / B; C = C / B; B = 1;
        }
        P_copper.line_set.push_back(line(p1, p2, A, B, C));
      } else if (str_list[0] == "arc") {
        double p1_x = stof(str_list.at(1));
        double p1_y = stof(str_list.at(2));
        double p2_x = stof(str_list.at(3));
        double p2_y = stof(str_list.at(4));
        double pc_x = stof(str_list.at(5));
        double pc_y = stof(str_list.at(6));
        double A = p2_y - p1_y;
        double B = p1_x - p2_x;
        double C = p1_x * p2_y - p2_x * p1_y;
        bool cw = 0;
        point p1(p1_x, p1_y);
        point p2(p2_x, p2_y);
        point pc(pc_x, pc_y);
        if (A < 0 || (A == 0 && B < 0)) {
          A *= -1; B *= -1; C *= -1;
        }
        if (A != 0) {
          B = B / A; C = C / A; A = 1;
        } else {
          A = A / B; C = C / B; B = 1;
        }
        if (str_list.at(7) == "CW") cw = 0;
        else cw = 1;
        P_copper.line_set.push_back(line(p1, p2, A, B, C));
        P_copper.arc_set.push_back(arc(p1, p2, pc, cw));
      }
    }
  }
  init_list.push_back(P_copper);
}

void graph::extend_polygon()
{
  for (const auto &p: init_list) {
    polygon new_p;

    new_p = extend(p);
    extended_list.push_back(new_p);
  }

  for (auto &p: extended_list) {
    int last_idx = p.line_set.size() - 1;
    for (int i = 0; i < p.line_set.size(); i++) {
      if (p.line_set[i].a != p.line_set[last_idx].a || p.line_set[i].b != p.line_set[last_idx].b) {
        point p_cross = GetCrossPoint(p.line_set[last_idx], p.line_set[i]);
        p.line_set[i].start = p_cross;
        p.line_set[last_idx].end = p_cross;
        last_idx = i;
      }
    }
    show_line_set(p);
  }
}

void graph::find_max()
{
  for (auto &p: init_list) {
    p.x_max = p.line_set[0].start.x;
    p.x_min = p.line_set[0].start.x;
    p.y_max = p.line_set[0].start.y;
    p.y_min = p.line_set[0].start.y;
    p.r_max = 0;
    for (const auto &l: p.line_set) {
      if (l.start.x < p.x_min) p.x_min = l.start.x;
      else if (l.start.x > p.x_max) p.x_max = l.start.x;
      if (l.start.y < p.y_min) p.y_min = l.start.y;
      else if (l.start.y > p.y_max) p.y_max = l.start.y;
    }
    for (const auto &a: p.arc_set) {
      if (a.radius > p.r_max) p.r_max = a.radius;
    }
    // if (p.type == 0) cout << "assembly" << endl;
    // else if (p.type == 1) cout << "copper" << endl;
    // cout << "x_max = " << p.x_max << endl;
    // cout << "x_min = " << p.x_min << endl;
    // cout << "y_max = " << p.y_max << endl;
    // cout << "y_min = " << p.y_min << endl;
    // cout << "r_max = " << p.r_max << endl;
  }

  x_max = init_list[0].x_max;
  x_min = init_list[0].x_min;
  y_max = init_list[0].y_max;
  y_min = init_list[0].y_min;
  r_max = 0;
  for (auto &p: init_list) {
    if (x_max < p.x_max) x_max = p.x_max;
    else if (x_min > p.x_min) x_min = p.x_min;
    if (y_max < p.y_max) y_max = p.y_max;
    else if (y_min > p.y_min) y_min = p.y_min;
    if (r_max < p.r_max) r_max = p.r_max;
  }

  // cout << "total" << endl;
  // cout << "x_max = " << x_max << endl;
  // cout << "x_min = " << x_min << endl;
  // cout << "y_max = " << y_max << endl;
  // cout << "y_min = " << y_min << endl;
  // cout << "r_max = " << r_max << endl;
}

void graph::show_polygons()
{
  for (const auto &p: init_list) {
    if (p.type == 0) cout << "assembly" << endl;
    else if (p.type == 1) cout << "copper" << endl;
    for (const auto &l: p.line_set) cout << "line," << l.start.x << "," << l.start.y << "," << l.end.x << "," << l.end.y << endl;
    for (const auto &a: p.arc_set) cout << "arc," << a.start.x << "," << a.start.y << "," << a.end.x << "," << a.end.y << "," << a.c.x << ',' << a.c.y << endl;
  }
}

void graph::show_line_set(polygon p)
{
  cout << "Line set" << endl;
  for (auto &l: p.line_set) l.show_line();
}

void graph::show_arc_set(polygon p)
{
  cout << "Arc set" << endl;
  for (auto &l: p.arc_set) l.show_arc();
}

void graph::show_result(ofstream &out_file)
{
  x_max += assemblygap + coppergap + 2 * r_max;
  x_min -= assemblygap + coppergap + 2 * r_max;
  y_max += assemblygap + coppergap + 2 * r_max;
  y_min -= assemblygap + coppergap + 2 * r_max;
  if (x_max - x_min < silkscreenlen) cout << "error" << endl;
  if (y_max - y_min < silkscreenlen) cout << "error" << endl;

  out_file << "silkscreen\n";
  out_file << "line," << setprecision(4) << fixed << x_max << "," << setprecision(4) << fixed << y_max << "," << setprecision(4) << fixed << x_max << "," << setprecision(4) << fixed << y_min << endl;
  out_file << "line," << setprecision(4) << fixed << x_max << "," << setprecision(4) << fixed << y_min << "," << setprecision(4) << fixed << x_min << "," << setprecision(4) << fixed << y_min << endl;
  out_file << "line," << setprecision(4) << fixed << x_min << "," << setprecision(4) << fixed << y_min << "," << setprecision(4) << fixed << x_min << "," << setprecision(4) << fixed << y_max << endl;
  out_file << "line," << setprecision(4) << fixed << x_min << "," << setprecision(4) << fixed << y_max << "," << setprecision(4) << fixed << x_max << "," << setprecision(4) << fixed << y_max << endl;
}

polygon graph::extend(const polygon p)
{
  polygon extended_p;

  for (const auto &l: p.line_set) {
    double vector_x = l.end.x - l.start.x;
    double vector_y = l.end.y - l.start.y;
    double angle = atan2(vector_y, vector_x) * 180 / MY_PI;
    double A = l.a;
    double B = l.b;
    double C = l.c;
    double bias = sqrt(assemblygap * assemblygap * (l.a * l.a + l.b * l.b));
    point p1(0, 0);
    point p2(0, 0);
    if (angle <= 0) C += bias;
    else C -= bias;
    extended_p.line_set.push_back(line(p1, p2, A, B, C));
    // cout << angle << endl;
  }

  return extended_p;
}

point graph::GetCrossPoint(line l1, line l2)
{
	point pTemp;
	double D;

  D = l1.a * l2.b - l2.a * l1.b;  
  pTemp.x = -1 * (l2.c * l1.b - l2.b * l1.c) / D;
  pTemp.y = -1 * (l1.c * l2.a - l2.c * l1.a) / D;
  return pTemp;
}

vector<string> graph::split(const string& str, const string& pattern) {
    vector<string> result;
    string::size_type begin, end;

    end = str.find(pattern);
    begin = 0;

    while (end != string::npos) {
        if (end - begin != 0) {
            result.push_back(str.substr(begin, end-begin)); 
        }    
        begin = end + pattern.size();
        end = str.find(pattern, begin);
    }

    if (begin != str.length()) {
        result.push_back(str.substr(begin));
    }
    return result;        
}
