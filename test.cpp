#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define KDTREE_2D_IMPLEMENTATION
#include "kdtree2d.h"

using namespace std;

int main(int argc, char** argv) {
	if (true) {
		kd2::Tree kd;
		int       dim = 50;
		kd.Initialize(0, 0, dim, dim);
		vector<kd2::Box> boxes;
		for (float x = 0; x < dim; x++) {
			for (float y = 0; y < dim; y++) {
				auto index = boxes.size();
				boxes.push_back({x + 0.1f, y + 0.1f, x + 0.9f, y + 0.9f});
				kd.Insert(index, x + 0.1f, y + 0.1f, x + 0.9f, y + 0.9f);
			}
		}

		for (int i = 0; i < 10000; i++) {
			if (i == 100) {
				// test copy constructor and operator=
				kd2::Tree copy = kd;
				kd             = copy;
			} else if (i == 200) {
				// test move
				kd2::Tree copy = kd;
				kd             = std::move(copy);
			}

			int      precision      = 100;
			int      maxQueryWindow = 5;
			kd2::Box query;
			query.X1 = (float) (rand() % (dim * precision)) / (float) precision;
			query.Y1 = (float) (rand() % (dim * precision)) / (float) precision;
			query.X2 = query.X1 + (float) (rand() % (maxQueryWindow * precision)) / (float) precision;
			query.Y2 = query.Y1 + (float) (rand() % (maxQueryWindow * precision)) / (float) precision;
			query.X1 = 0;
			query.Y1 = 0;
			query.X2 = 3;
			query.Y2 = 3;
			//kd.NScanned  = 0;
			auto results = kd.Find(query);
			//printf("Scanned: %d. Results: %d\n", (int) kd.NScanned, (int) results.size());
			// brute force validation that there are no false negatives
			for (size_t j = 0; j < boxes.size(); j++) {
				if (boxes[j].Overlaps(query)) {
					// if object overlaps the query rectangle, then it should be included in the result set
					size_t k = 0;
					for (; k < results.size(); k++) {
						if (results[k] == j)
							break;
					}
					if (k == results.size()) {
						printf("error\n");
						exit(1);
					}
				}
			}
		}
		printf("Validation OK\n");
	}
	{
		printf("Benchmark:\n");
		kd2::Tree kd;
		int       dim   = 1000;
		auto      start = clock();
		kd.Initialize(0, 0, dim, dim);
		size_t index = 0;
		for (float x = 0; x < dim; x++) {
			for (float y = 0; y < dim; y++)
				kd.Insert(index++, x + 0.1, y + 0.1, x + 0.9, y + 0.9);
		}
		printf("Time to insert %d elements: %.0f milliseconds\n", dim * dim, 1000 * (clock() - start) / (double) CLOCKS_PER_SEC);

		start                      = clock();
		size_t              nquery = 10 * 1000 * 1000;
		std::vector<size_t> results;
		int                 sx       = 0;
		int                 sy       = 0;
		int64_t             nresults = 0;
		for (size_t i = 0; i < nquery; i++) {
			results.clear();
			float minx = sx % dim;
			float miny = sy % dim;
			float maxx = minx + 5.0;
			float maxy = miny + 5.0;
			//kd.NScanned = 0;
			kd.Find({minx, miny, maxx, maxy}, results);
			//printf("Scanned: %d. Results: %d\n", (int) kd.NScanned, (int) results.size());
			nresults += results.size();
		}
		printf("Time per query, returning average of %.0f elements: %.2f nanoseconds\n", (double) nresults / (double) nquery, (1000000000.0 / (double) nquery) * (clock() - start) / (double) CLOCKS_PER_SEC);
	}
	return 0;
}