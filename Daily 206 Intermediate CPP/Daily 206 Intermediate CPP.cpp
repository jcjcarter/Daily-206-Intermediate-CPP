// Daily 206 Intermediate CPP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <amp.h>

using namespace std;
namespace con = concurrency;

int main()
{

	int w, h, r;
	vector<int> vec;

	if (!(cin >> h >> w >> r))
	{
		cout << "error!" << endl;
	}

	vec.reserve(w*h);

	copy_n(istream_iterator<char>(cin), w*h, back_inserter(vec));

	const int rr = r * r;
	con::array_view<int, 1> crops_idx(w * h);
	con::array_view<int, 1> crops(w*h);

	con::array<int, 2> field(h,w);
	con::array_view<const int, 2> input(h,w, vec);

	int values[] = { 0,0,0 };

	con::array_view<int, 1> view_max(values);

	con::parallel_for_each(field.extent, [=, &crops](con::index<2> idx) restrict(amp) {
	
		int my_field = input[idx];
		if (my_field == 'x')
		{
			int my_idx = con::atomic_fetch_inc(&crops_idx[0]);
			crops[my_idx] = (idx[0] << 16) | idx[1];
		}

	});


	con::parallel_for_each(field.extent, [=, &crops, &field](con::index<2> idx) restrict(amp)
	{
	
		int my_value = 0;
		int crops_cnt = crops_idx[0];

		for (int i = 0; i < crops_cnt; ++i) {
			int tmp = crops[i];
			int y = tmp >> 16;
			int x = tmp & 0xFFFF;

			int dy = idx[0] - y;
			int dx = idx[1] - x;

			my_value += (dx*dx + dy*dy <= rr) && (x != idx[1] ||
				y != idx[0]);
		}

		field[idx] = my_value;
		con::atomic_fetch_max(&view_max(0), my_value);

	});

	con::parallel_for_each(field.extent, [&field, view_max](con::index<2> idx)
		restrict(amp) {
		
		int my_val = field[idx];
		if (my_val == view_max[0] && con::atomic_exchange(&view_max[2], 1) == 0)
		{
			view_max[1] = (idx[0] << 16) | idx[1];
		}
	});


	cout << "best is " << view_max(0) << " of " << crops_idx(0) << " crops at {"
		<< (view_max(1) >> 16) << ", " << (view_max(1) & 0xFFFF) << "}" << endl;

    return 0;
}

