#pragma once

#include <vector>

template<typename T>
std::vector<T> setDifference(const std::vector<T>& as, const std::vector<T>& bs) {
	std::vector<T> difference;
	for (const auto& a : as) {
		bool aInBs = false;
		for (const auto& b : bs) {
			if (a == b) {
				aInBs = true;
				break;
			}
		}
		if (!aInBs) {
			difference.push_back(a);
		}
	}
	return difference;
}