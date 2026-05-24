#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
using namespace std;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT
#endif

struct Line {
    long double k, b;
    bool operator==(const Line& other) const {
        return k == other.k && b == other.b;
    }
    
    long double get(long double x) {
    	return k * x + b;
	}
};

struct LineContainer {
    vector<Line> hullMin, hullMax, allLines;
    vector <long double> dots;
    bool maxIsUsed = true;

    long double intersection(Line a, Line b) {
        return (a.b - b.b) / (b.k - a.k);
    }

    static bool comp(const Line &a, const Line &b) {
        if(a.k == b.k) return a.b < b.b;
        return a.k < b.k;
    }

    void updateTheHull() {
        {
            vector<Line> copy = allLines;
            hullMax.clear();
            sort(copy.begin(), copy.end(), comp);
            for(Line x : copy) {
                while(!hullMax.empty() && hullMax.back().k == x.k)
                    hullMax.pop_back();
                while(hullMax.size() >= 2) {
                    Line last = hullMax.back(), comparing = hullMax[hullMax.size() - 2];
                    if(intersection(comparing, x) <= intersection(comparing, last))
                        hullMax.pop_back();
                    else break;
                }
                hullMax.push_back(x);
            }
        }
        {
            vector<Line> copy = allLines;
            hullMin.clear();
            sort(copy.begin(), copy.end(), comp);
            reverse(copy.begin(), copy.end());
            for(Line x : copy) {
                while(!hullMin.empty() && hullMin.back().k == x.k)
                    hullMin.pop_back();
                while(hullMin.size() >= 2) {
                    Line last = hullMin.back(), comparing = hullMin[hullMin.size() - 2];
                    if(intersection(comparing, x) <= intersection(comparing, last))
                        hullMin.pop_back();
                    else break;
                }
                hullMin.push_back(x);
            }
        }
    }

    void addLine(Line newLine) {
        allLines.push_back(newLine);
        updateTheHull();
    }
	
	void addDot(long double x) {
		dots.push_back(x);
	}
	
	void remDot(long double x) {
		for(int i = 0; i < dots.size(); i ++) {
			if(dots[i] == x) {
				dots.erase(dots.begin() + i);
				return;
			}
		}
	}
	
	long double getVal(long double x) {
		if(maxIsUsed) {
			long double res = -1e18;
			for(int i = 0; i < hullMax.size(); i ++) {
				res = max(res, hullMax[i].get(x));
			}
			return res;
		}
		else{
			long double res = 1e18;
			for(int i = 0; i < hullMin.size(); i ++) {
				res = min(res, hullMin[i].get(x));
			}
			return res;
		}
	}
	
    void deleteLine(Line remLine) {
        for(int i = 0; i < allLines.size(); i++) {
            if(allLines[i] == remLine) {
                allLines.erase(allLines.begin() + i);
                break;
            }
        }
        updateTheHull();
    }

    void switchMinMax() { maxIsUsed = !maxIsUsed; }

    bool checkIfInHull(Line x) {
        auto& hull = maxIsUsed ? hullMax : hullMin;
        for(auto& l : hull) if(l == x) return true;
        return false;
    }

    struct Segment { long double k, b, from, to; bool active; };
    vector<Segment> segments;

    void draw(long double lef, long double rig, long double k, long double b, bool active) {
        if(lef >= rig) return;
        segments.push_back({k, b, lef, rig, active});
    }

    string output() {
    	
        string res = "{\"segments\":[";
        for(int i = 0; i < segments.size(); i++) {
            res += "{";
            res += "\"k\":"    + to_string((double)segments[i].k) + ",";
            res += "\"b\":"    + to_string((double)segments[i].b) + ",";
            res += "\"from\":" + to_string((double)segments[i].from) + ",";
            res += "\"to\":"   + to_string((double)segments[i].to) + ",";
            res += "\"active\":";
            res += segments[i].active ? "true" : "false";
            res += "}";
            if(i + 1 < segments.size()) res += ",";
        }
        res += "], \"points\":[";
        if(!segments.empty()) {
        	for(int i = 0; i < dots.size(); i ++) { 
        		long double x = dots[i];
        		res += "{\"x\":" + to_string((double)x) + ",\"y\":" + to_string((double)getVal(x)) + "}";
        		if(i + 1 != dots.size()) {
        			res += ",";
				}
			}
		}
		res += "]}";
        return res;
    }

    string visualize(long double rangeMin, long double rangeMax) {
        segments.clear();
        auto& hull = maxIsUsed ? hullMax : hullMin;
        int n = hull.size();

        if(n == 0) return output();

        if(n == 1) {
            draw(rangeMin, rangeMax, hull[0].k, hull[0].b, true);
            return output();
        }

        for(int i = 1; i + 1 < n; i++) {
            Line cur = hull[i], prev = hull[i-1], nxt = hull[i+1];
            long double L = max(rangeMin, intersection(cur, prev));
            long double R = min(rangeMax, intersection(cur, nxt));
            draw(L, R, cur.k, cur.b, 1);
            draw(rangeMin, L, cur.k, cur.b, 0);
            draw(R, rangeMax, cur.k, cur.b, 0);
        }

        draw(rangeMin, intersection(hull[0], hull[1]), hull[0].k, hull[0].b, 1);
        draw(intersection(hull[0], hull[1]), rangeMax, hull[0].k, hull[0].b, 0);

        draw(rangeMin, intersection(hull[n-1], hull[n-2]), hull[n-1].k, hull[n-1].b, 0);
        draw(intersection(hull[n-1], hull[n-2]), rangeMax, hull[n-1].k, hull[n-1].b, 1);

        for(auto& line : allLines)
            if(!checkIfInHull(line))
                draw(rangeMin, rangeMax, line.k, line.b, 0);

        return output();
    }
};

// глобальный экземпляр
LineContainer lc;
static string lastResult; // буфер для WASM (c_str() должен жить достаточно долго)

extern "C" {
    EXPORT void   addLine(double k, double b)    { lc.addLine({k, b}); }
    EXPORT void   deleteLine(double k, double b) { lc.deleteLine({(long double)k, (long double)b}); }
    EXPORT void   switchMinMax()                 { lc.switchMinMax(); }
    EXPORT void   clearAll()                     { lc = LineContainer(); }
    EXPORT void   addDot(double x)               { lc.addDot(x); } 
    EXPORT void   remDot(double x)               { lc.remDot(x); } 

    EXPORT const char* visualize(double rangeMin, double rangeMax) {
        lastResult = lc.visualize(rangeMin, rangeMax);
        return lastResult.c_str();
    }
}

// для локального тестирования
#ifndef __EMSCRIPTEN__
int main() {
    int n;
    cin >> n;
    for(int i = 0; i < n; i++) {
        double k, b;
        cin >> k >> b;
        lc.addLine({k, b});
    }
    double rangeMin, rangeMax;
    cin >> rangeMin >> rangeMax;
    cout << lc.visualize(rangeMin, rangeMax) << endl;
    return 0;
}
#endif