// Copyright 2022 Wook-shin Han
#include "./project1.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

std::vector<int> string2int_vector(std::vector<std::string> v) {
  std::vector<int> iv;
  for (std::string s : v) {
    iv.push_back(std::stoi(s));
  }
  return iv;
}

std::vector<int> get_start_positions(std::string column_names) {
  int length = column_names.length();
  bool is_previous_char_space = true;
  std::vector<int> positions;
  for (int i = 0; i < length; i++) {
    if (i == length - 1) continue;
    if (is_previous_char_space && column_names[i] != ' ') {
      positions.push_back(i);
    }
    is_previous_char_space = column_names[i] == ' ';
  }
  positions.push_back(length);
  return positions;
}

std::map<int, int> get_buyer_number(std::vector<std::string> buyers,
                                    std::vector<int> barcodes,
                                    std::vector<int> quantities) {
  std::map<int, std::set<std::string>> unique_buyers;
  std::map<int, int> buyer_number;
  if (buyers.size() != barcodes.size() || barcodes.size() != quantities.size())
    return std::map<int, int>();

  int length = buyers.size();
  for (int i = 0; i < length; i++) {
    if (unique_buyers.find(barcodes[i]) == unique_buyers.end()) {
      std::set<std::string> buyer;
      unique_buyers[barcodes[i]] = buyer;
    }
    unique_buyers[barcodes[i]].insert(buyers[i]);
  }

  for (auto& pair : unique_buyers) {
    buyer_number[pair.first] = pair.second.size();
  }
  return buyer_number;
}

std::vector<std::string> extract_column(std::string input, int column) {
  std::string parsing_line;
  std::vector<std::string> result;
  std::ifstream input_stream(input);
  if (!input_stream.is_open()) {
    std::cout << "wrong file name\n";
    return std::vector<std::string>();
  }

  std::getline(input_stream, parsing_line);
  std::vector<int> positions = get_start_positions(parsing_line);
  std::getline(input_stream, parsing_line);  // drop separator
  std::string white_space = " \t";

  while (std::getline(input_stream, parsing_line)) {
    int length = positions.size();
    int idx = 0;
    for (int i = 0; i < length; i++) {
      if (i == length - 1) continue;
      if (i == column) {
        std::string data =
            parsing_line.substr(positions[i], positions[i + 1] - positions[i]);
        size_t end = data.find_last_not_of(white_space);
        std::string trimmed_data = data.substr(0, end + 1);
        result.push_back(trimmed_data);
      }
    }
  }

  return result;
}

int main(int argc, char** argv) {
  /* fill this */
  /* SELECT LAST_NAME FROM CUSTOMER
          WHERE ACTIVE = 1 AND
          ZONE IN (
                  SELECT ZONE_ID FROM ZONECOST WHERE ZONE_DESC = TORONTO
          ); */
  // : <your_binary> q1 <customer.file> <zonecost.file>
  if (strcmp(argv[1], "q1") == 0) {
    std::string customer_table = argv[2];
    std::string zone_cost_table = argv[3];
    std::string parsing_line;

    std::vector<int> zone_ids =
        string2int_vector(extract_column(zone_cost_table, 0));
    std::vector<std::string> zone_desc = extract_column(zone_cost_table, 1);

    if (zone_desc.size() != zone_ids.size()) {
      std::cout << "zone file parsing error!\n";
      return 0;
    }

    std::set<int> toronto_ids;
    for (size_t i = 0; i < zone_desc.size(); i++) {
      if (strcmp(zone_desc[i].c_str(), "Toronto") == 0)
        toronto_ids.insert(zone_ids[i]);
    }

    std::vector<int> active_status =
        string2int_vector(extract_column(customer_table, 12));
    std::vector<std::string> last_names = extract_column(customer_table, 2);
    std::vector<int> customer_zones =
        string2int_vector(extract_column(customer_table, 5));

    if (active_status.size() != last_names.size()) {
      std::cout << "customer file parsing error!\n";
      return 0;
    }

    std::vector<std::string> satisfied_last_names;
    for (size_t i = 0; i < last_names.size(); i++) {
      auto is_contain = toronto_ids.find(customer_zones[i]);
      if (active_status[i] == 1 && is_contain != toronto_ids.end())
        satisfied_last_names.push_back(last_names[i]);
    }

    for (std::string last_name : satisfied_last_names) {
      std::cout << last_name << "\n";
    }
  } else if (strcmp(argv[1], "q2") == 0) {
    // Output the BARCODE and the PRODDESC for each product that has been
    // purchased by at least two customers.
    //  <your_binary> q2 <lineitem.file> <products.file>
    /* SELECT BARCODE, PRODDESC FROM PRODUCTS
    * WHERE BARCODE IN (
            SELECT BARCODE
            FROM LINEITEM
            GROUP BY BARCODE
            HAVING COUNT(DISTINCT UNAME) >= 2
    )
    */
    std::string lineitem_table = argv[2];
    std::string product_table = argv[3];

    std::vector<std::string> user_name = extract_column(lineitem_table, 0);
    std::vector<int> barcodes =
        string2int_vector(extract_column(lineitem_table, 3));
    std::vector<int> quantities =
        string2int_vector(extract_column(lineitem_table, 4));
    std::map<int, int> unique_buyers =
        get_buyer_number(user_name, barcodes, quantities);

    std::vector<int> existing_barcodes =
        string2int_vector(extract_column(product_table, 0));
    std::vector<std::string> products = extract_column(product_table, 2);
    std::vector<int> bought_existing_barcodes;
    std::vector<std::string> bought_products;

    for (size_t i = 0; i < existing_barcodes.size(); i++) {
      int barcode = existing_barcodes[i];
      if (unique_buyers.find(barcode) == unique_buyers.end())
        continue;
      else if (unique_buyers[barcode] >= 2) {
        bought_existing_barcodes.push_back(barcode);
        bought_products.push_back(products[i]);
      }
    }

    for (size_t i = 0; i < bought_existing_barcodes.size(); i++) {
      std::cout << bought_existing_barcodes[i] << "                 "
                << bought_products[i] << "\n";
    }
  }

  return 0;
}
