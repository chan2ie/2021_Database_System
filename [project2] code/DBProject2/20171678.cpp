#include <iostream>
#include <fstream>
#include <string>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "mysql.h"

#pragma comment(lib, "libmysql.lib")

const char* host = "localhost";
const char* user = "root";
const char* pw = "password";
const char* db = "project_db";

MYSQL* con;
MYSQL mysql;
MYSQL_RES* sql_res;
MYSQL_ROW sql_row;
FILE* fp;

struct tm today;

using namespace std;

int send_query_from_file(const char* filename) {
	string query;

	ifstream fileStream(filename);

	if (!fileStream.is_open()) {
		cout << "Query execution failed: failed to open '" << filename << "'." << endl;
		return 1;
	}

	query.reserve(500);

	while (!fileStream.eof()) {
		string temp;
		getline(fileStream, temp);
		query.append(temp);

		if (query.empty()) continue;

		if (query.back() == ';') { // query fully read
			if (mysql_query(con, query.c_str()) == 0) // send query
				sql_res = mysql_store_result(con);
			else {
				cout << "Query execution failed (" << mysql_errno(&mysql) << "): " << mysql_error(&mysql) << endl;
				return 1;
			}
			query.clear();
		}
	}

	fileStream.close();

	return 0;
}

string string_format(const std::string str, ...) {
	int size = ((int)str.size()) * 2;
	string buf;
	va_list ap;
	while (1) {
		buf.resize(size);
		va_start(ap, str);
		int n = vsnprintf((char*)buf.data(), size, str.c_str(), ap);
		va_end(ap);
		if (n > -1 && n < size) {
			buf.resize(n);
			return buf;
		}
		if (n > -1)
			size = n + 1;
		else
			size *= 2;
	}
	return buf;
}

void t1() {
	string query;
	int brand = 0;
	const char* brands[3] = { "Hyundai", "Hyundai Truck & Bus", "Genesis" };
	int k = 0;
	int subtype = 0;
	int subsubtype = 0;
	int income = -1;

	cout << "\n------- TYPE 1 -------\n" << endl;
	cout << "** Show the sales trends for a particular brand over the past k years." << endl;
	while (brand == 0 || brand > 3) {
		cout << " Which Brand?\n\t1. Hyundai\n\t2. Hyundai Truck & Bus\n\t3. Genesis\n Brand: ";
		cin >> brand;
	}

	while (k <= 0) {
		cout << " Which K?: ";
		cin >> k;
	}

	query = string_format("\
		SELECT YEAR(t.ts_date) as year, SUM(t.p) as sale\n\
		FROM(\n\
			SELECT VIN, ts_date, brand, model.M_name, model_spec.price + ifnull(add_price, 0) as p, color\n\
			FROM vehicle\n\
			LEFT JOIN model_spec on model_spec.M_ID = vehicle.M_ID\n\
			LEFT JOIN colors on vehicle.M_ID = colors.M_ID and vehicle.O_ID = colors.O_ID\n\
			LEFT JOIN model on model_spec.M_name = model.M_name\n\
			WHERE ts_date between '%d-01-01' AND '2020-12-31' AND brand = '%s'\n\
		) as t\n\
		GROUP BY YEAR(ts_date)\n\
		ORDER BY year", today.tm_year + 1900 - k, brands[brand - 1]);

	if (mysql_query(con, query.c_str()) == 0) {
		cout << "\n |  YEAR  |    SALES    |" << endl;
		cout << " ------------------------" << endl;
		sql_res = mysql_store_result(con);
		while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
			printf(" |  %s  |%12s |\n", sql_row[0], sql_row[1]);
		}
		mysql_free_result(sql_res);
	}

	while (subtype != 1) {
		cout << "\n------- Subtypes in TYPE 1 -------\n" << endl;
		cout << "\t1. TYPE 1-1" << endl;
		cout << "\t0. QUIT\nTYPE: ";
		cin >> subtype;
	
		if (!subtype) return;
	}

	query = string_format("\
		SELECT YEAR(t.ts_date) as year, gender, SUM(t.p)\n\
		FROM(\n\
			SELECT gender, ts_date, brand, model_spec.price + ifnull(add_price, 0) as p\n\
			FROM vehicle\n\
			LEFT JOIN model_spec on model_spec.M_ID = vehicle.M_ID\n\
			LEFT JOIN colors on vehicle.M_ID = colors.M_ID and vehicle.O_ID = colors.O_ID\n\
			LEFT JOIN model on model_spec.M_name = model.M_name\n\
			LEFT JOIN customer on customer.C_ID = vehicle.C_ID\n\
			WHERE ts_date between '%d-01-01' AND '2020-12-31' AND brand = '%s'\n\
		) as t\n\
		GROUP BY YEAR(ts_date), gender\n\
		ORDER BY year, GENDER\n\
		", today.tm_year + 1900 - k, brands[brand - 1]);

	cout << "\n------- TYPE 1-1 -------\n" << endl;
	cout << "** Break these data out by gender of the buyer." << endl;

	if (mysql_query(con, query.c_str()) == 0) {
		cout << "\n |  YEAR  |  GENDER  |    SALES    |" << endl;
		cout << " -----------------------------------" << endl;
		sql_res = mysql_store_result(con);
		while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
			printf(" |  %s  | %8s | %11s |\n", sql_row[0], sql_row[1], sql_row[2]);
		}
		mysql_free_result(sql_res);
	}

	while (subsubtype != 1) {
		cout << "\n------- Subtypes in TYPE 1-1 -------\n" << endl;
		cout << "\t1. TYPE 1-1-1" << endl;
		cout << "\t0. QUIT\nTYPE: ";
		cin >> subsubtype;

		if (!subsubtype) return;
	}

	cout << "\n------- TYPE 1-1-1 -------\n" << endl;
	cout << "** Break these data out by income range." << endl;
	while (income < 0) {
		cout << " Enter the criteria for income (Unit: KRW 10,000): ";
		cin >> income;
	}

	query = string_format("\
		SELECT YEAR(t.ts_date) as year, gender, SUM(t.p), income\n\
		FROM(\n\
			SELECT gender, ts_date, brand, model_spec.price + ifnull(add_price, 0) as p,\n\
			CASE WHEN income_range is null THEN null WHEN income_range < %d0000 THEN 0 ELSE 1 END as income, income_range\n\
			FROM vehicle\n\
			LEFT JOIN model_spec on model_spec.M_ID = vehicle.M_ID\n\
			LEFT JOIN colors on vehicle.M_ID = colors.M_ID and vehicle.O_ID = colors.O_ID\n\
			LEFT JOIN model on model_spec.M_name = model.M_name\n\
			LEFT JOIN customer on customer.C_ID = vehicle.C_ID\n\
			WHERE ts_date between '%d-01-01' AND '2020-12-31' AND brand = '%s'\n\
		) as t\n\
		GROUP BY YEAR(ts_date), gender, income\n\
		ORDER BY year, GENDER\n\
		", income, today.tm_year + 1900 - k, brands[brand - 1]);

	if (mysql_query(con, query.c_str()) == 0) {
		cout << "\n |  YEAR  |  GENDER  | INCOME RANGE |    SALES    |" << endl;
		cout << " --------------------------------------------------" << endl;
		sql_res = mysql_store_result(con);
		while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
			string income_range;
			if (!sql_row[3]) income_range = "(null)";
			else if (sql_row[3][0] == '1') {
				income_range = string_format(">= %d", income);
			}
			else income_range = string_format("< %d", income);

			printf(" |  %s  | %8s | %12s | %11s |\n", sql_row[0], sql_row[1], income_range.c_str(), sql_row[2]);
		}
		mysql_free_result(sql_res);
	}

}

void t2() {
	string query;
	int k = 0;
	int subtype = 0;
	int subsubtype = 0;
	int income = -1;

	cout << "\n------- TYPE 2 -------\n" << endl;
	cout << "** Show sales trends for various brands over the past k months." << endl;

	while (k <= 0) {
		cout << " Which K?: ";
		cin >> k;
	}

	int year = today.tm_year + 1900;
	int month = today.tm_mon;
	
	for (int i = 0; i < k; i++) {
		if (month == 1) {
			year--;
			month = 12;
		}
		else
			month--;
	}

	query = string_format("\
		SELECT brand, year(ts_date), month(ts_date), SUM(P)\n\
		FROM(\n\
			SELECT ts_date, brand, price + ifnull(add_price, 0) as p\n\
			FROM vehicle left join model_spec on model_spec.M_ID = vehicle.M_ID\n\
			left join model on model_spec.M_name = model.M_name\n\
			left join colors on vehicle.O_ID = colors.O_ID\n\
			WHERE ts_date >= '%d-%d-01'\n\
		) as t\n\
		group by year(ts_date), month(ts_date), brand\n\
		order by brand, year(ts_date), month(ts_date)\n\
		", year, month);

	char brand[20] = " ";

	if (mysql_query(con, query.c_str()) == 0) {
		sql_res = mysql_store_result(con);
		while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
			if (strcmp(brand, sql_row[0]) != 0) {
				strcpy_s(brand, 20, sql_row[0]);
				cout << "\n << " << sql_row[0] << " >>\n" << endl;
				cout << " |  MONTH  |    SALES    |" << endl;
				cout << " -------------------------" << endl;
			}

			printf(" | %4s.%02s |%12s |\n", sql_row[1], sql_row[2], sql_row[3]);
		}
		mysql_free_result(sql_res);
	}

	while (subtype != 1) {
		cout << "\n------- Subtypes in TYPE 2 -------\n" << endl;
		cout << "\t1. TYPE 2-1" << endl;
		cout << "\t0. QUIT\nTYPE: ";
		cin >> subtype;

		if (!subtype) return;
	}

	query = string_format("\
		SELECT brand, year(ts_date), month(ts_date), gender, SUM(P)\n\
		FROM(\n\
			SELECT ts_date, brand, price + ifnull(add_price, 0) as p, gender\n\
			FROM vehicle left join model_spec on model_spec.M_ID = vehicle.M_ID\n\
			left join model on model_spec.M_name = model.M_name\n\
			left join colors on vehicle.O_ID = colors.O_ID\n\
			LEFT JOIN customer on customer.C_ID = vehicle.C_ID\n\
			WHERE ts_date >= '%d-%d-01'\n\
		) as t\n\
		group by year(ts_date), month(ts_date), brand, gender\n\
		order by brand, year(ts_date) DESC, month(ts_date)\n\
		", year, month);

	cout << "\n------- TYPE 2-1 -------\n" << endl;
	cout << "** Break these data out by gender of the buyer." << endl;

	strcpy_s(brand, 20, " ");

	if (mysql_query(con, query.c_str()) == 0) {
		sql_res = mysql_store_result(con);
		while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
			if (strcmp(brand, sql_row[0]) != 0) {
				strcpy_s(brand, 20, sql_row[0]);
				cout << "\n << " << sql_row[0] << " >>\n" << endl;
				cout << " |  MONTH  |  GENDER  |    SALES    |" << endl;
				cout << " ------------------------------------" << endl;
			}

			printf(" | %4s.%02s | %8s |%12s |\n", sql_row[1], sql_row[2], sql_row[3], sql_row[4]);
		}
		mysql_free_result(sql_res);
	}


	while (subsubtype != 1) {
		cout << "\n------- Subtypes in TYPE 2-1 -------\n" << endl;
		cout << "\t1. TYPE 2-1-1" << endl;
		cout << "\t0. QUIT\nTYPE: ";
		cin >> subsubtype;

		if (!subsubtype) return;
	}

	cout << "\n------- TYPE 2-1-1 -------\n" << endl;
	cout << "** Break these data out by income range." << endl;
	while (income < 0) {
		cout << " Enter the criteria for income (Unit: KRW 10,000): ";
		cin >> income;
	}

	query = string_format("\
		SELECT brand, year(ts_date), month(ts_date), gender, SUM(P), income\n\
		FROM(\n\
			SELECT ts_date, brand, price + ifnull(add_price, 0) as p, gender,\n\
			CASE WHEN income_range is null THEN null WHEN income_range < %d0000 THEN 0 ELSE 1 END as income, income_range\n\
			FROM vehicle left join model_spec on model_spec.M_ID = vehicle.M_ID\n\
			left join model on model_spec.M_name = model.M_name\n\
			left join colors on vehicle.O_ID = colors.O_ID\n\
			LEFT JOIN customer on customer.C_ID = vehicle.C_ID\n\
			WHERE ts_date >= '%d-%d-01'\n\
		) as t\n\
		group by year(ts_date), month(ts_date), brand, gender, income\n\
		order by brand, year(ts_date) DESC, month(ts_date)\n\
		", income, year, month);

	strcpy_s(brand, 20, " ");

	if (mysql_query(con, query.c_str()) == 0) {
		sql_res = mysql_store_result(con);
		while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
			if (strcmp(brand, sql_row[0]) != 0) {
				strcpy_s(brand, 20, sql_row[0]);
				cout << "\n << " << sql_row[0] << " >>\n" << endl;
				cout << " |  MONTH  |  GENDER  | INCOME RANGE |    SALES    |" << endl;
				cout << " ---------------------------------------------------" << endl;
			}
			string income_range;
			if (!sql_row[5]) income_range = "(null)";
			else if (sql_row[5][0] == '1') {
				income_range = string_format(">= %d", income);
			}
			else income_range = string_format("< %d", income);

			printf(" | %4s.%02s | %8s | %12s |%12s |\n", sql_row[1], sql_row[2], sql_row[3], income_range.c_str(), sql_row[4]);
		}
		mysql_free_result(sql_res);
	}
}

void t3() {
	string query;
	string start;
	string end;
	string supplier;

	int subtype = 3;

	cout << "\n------- TYPE 3 -------\n" << endl;
	cout << "**  Find that transmissions made by supplier (company name) between two given dates are defective." << endl;
	
	cout << " Enter start date (ex. 2019-03-14): ";
	cin >> start;
	cout << " Enter end date (ex. 2019-03-14): ";
	cin >> end;

	cout << " Enter the name of the supplier (Among Supplier1 ~ Supplier15): ";
	cin >> supplier;

	query = string_format("\
		SELECT P_name, prod_date, supp_date\n\
		FROM supplier left join part on part.S_ID = supplier.S_ID\n\
		WHERE S_name = '%s' and\n\
		prod_date between '%s' AND '%s'\n\
		ORDER BY prod_date\n\
		", supplier.c_str(), start.c_str(), end.c_str());
	
	if (mysql_query(con, query.c_str()) == 0) {
		cout << "\n |  NAMES  | PRODUCED DATE | SUPPLIED DATE |" << endl;
		cout << " ------------------------------------------" << endl;
		sql_res = mysql_store_result(con);
		while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
			printf(" | %7s | %13s | %13s |\n", sql_row[0], sql_row[1], sql_row[2]);
		}
		mysql_free_result(sql_res);
	}

	while (subtype != 1 && subtype != 2) {
		cout << "\n------- Subtypes in TYPE 3 -------\n" << endl;
		cout << "\t1. TYPE 3-1" << endl;
		cout << "\t2. TYPE 3-2" << endl;
		cout << "\t0. QUIT\nTYPE: ";
		cin >> subtype;

		if (!subtype) return;

		if (subtype == 1) { // type 3-1
			cout << "\n------- TYPE 3-1 -------\n" << endl;
			cout << "** Find the VIN of each car containing such a transmission and the customer to which it was sold." << endl;

			query = string_format("\n\
			SELECT P_name, vehicle.VIN, C_name, phone_number\n\
			FROM vehicle left join mf_his on vehicle.VIN = mf_his.VIN\n\
			left join part on part.P_ID = mf_his.P_ID\n\
			left join supplier on part.S_ID = supplier.S_ID\n\
			left join customer on vehicle.C_ID = customer.C_ID\n\
			WHERE S_name = '%s' and\n\
			prod_date between '%s' AND '%s'\n\
			ORDER BY prod_date\n\
			", supplier.c_str(), start.c_str(), end.c_str());

			if (mysql_query(con, query.c_str()) == 0) {
				cout << "\n |  NAMES  |      VIN      |  CUSTOMER  |     CONTACT     |" << endl;
				cout << " ----------------------------------------------------------" << endl;
				sql_res = mysql_store_result(con);
				while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
					printf(" | %7s | %013s | %9s | %15s |\n", sql_row[0], sql_row[1], sql_row[2], sql_row[3]);
				}
				mysql_free_result(sql_res);
			}
		}
		if (subtype == 2) { // type 3-2
			cout << "\n------- TYPE 3-2 -------\n" << endl;
			cout << "** Find the dealer who sold the VIN and transmission for each vehicle containing these transmissions. " << endl;

			query = string_format("\n\
			SELECT P_name, vehicle.VIN, D_name\n\
			FROM vehicle left join mf_his on vehicle.VIN = mf_his.VIN\n\
			left join part on part.P_ID = mf_his.P_ID\n\
			left join supplier on part.S_ID = supplier.S_ID\n\
			left join dealer on vehicle.D_ID = dealer.D_ID\n\
			WHERE S_name = '%s' and\n\
			prod_date between '%s' AND '%s'\n\
			ORDER BY prod_date\n\
			", supplier.c_str(), start.c_str(), end.c_str());

			if (mysql_query(con, query.c_str()) == 0) {
				cout << "\n |  NAMES  |      VIN      |  DEALER  |" << endl;
				cout << " --------------------------------------" << endl;
				sql_res = mysql_store_result(con);
				while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
					printf(" | %7s | %013s | %9s |\n", sql_row[0], sql_row[1], sql_row[2]);
				}
				mysql_free_result(sql_res);
			}
		}
	}
}

void t4() {
	string query;
	int rank = 0;
	int year = 0;

	cout << "\n------- TYPE 4 -------\n" << endl;
	cout << "** Find the top k brands by dollar-amount sold by the year." << endl;

	while (rank <= 0) {
		cout << " Which K?: ";
		cin >> rank;
	}

	while (year <= 2010) {
		cout << "Since when? (Year): ";
		cin >> year;
	}

	query = string_format("\
		SELECT year(ts_date), brand, SUM(P)\n\
		FROM(\n\
			SELECT ts_date, brand, price + ifnull(add_price, 0) as p\n\
			FROM vehicle left join model_spec on model_spec.M_ID = vehicle.M_ID\n\
			left join model on model_spec.M_name = model.M_name\n\
			left join colors on vehicle.O_ID = colors.O_ID\n\
			WHERE year(ts_date) >= %d\n\
		) as t\n\
		group by year(ts_date), brand\n\
		order by year(ts_date), SUM(P) DESC\n\
		", year);

	int i = 1;
	char last_year[10] = "1900";

	if (mysql_query(con, query.c_str()) == 0) {
		cout << "\n    |         BRAND         |    SALES    |" << endl;
		sql_res = mysql_store_result(con);
		while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
			if (strcmp(last_year, sql_row[0]) != 0) {
				strcpy_s(last_year, 10, sql_row[0]);
				cout << " " << sql_row[0] << " -------------------------------------" << endl;
				i = 1;
			}
			if (i > rank) continue;
			printf(" %2d | %21s | %11s |\n", i, sql_row[1], sql_row[2]);
			i++;
		}
		mysql_free_result(sql_res);
	}
}

void t5() {
	string query;
	int rank = 0;
	int year = 0;

	cout << "\n------- TYPE 5 -------\n" << endl;
	cout << "** Find the top k brands by unit sales by the year." << endl;

	while (rank <= 0) {
		cout << " Which K?: ";
		cin >> rank;
	}

	while (year <= 2010) {
		cout << " Since when? (Year): ";
		cin >> year;
	}

	query = string_format("\
		SELECT year(ts_date), brand, COUNT(*)\n\
		FROM(\n\
			SELECT ts_date, brand\n\
			FROM vehicle left join model_spec on model_spec.M_ID = vehicle.M_ID\n\
			left join model on model_spec.M_name = model.M_name\n\
			WHERE year(ts_date) >= %d\n\
		) as t\n\
		group by year(ts_date), brand\n\
		order by year(ts_date), count(*) DESC", year);

	int i = 1;
	char last_year[10] = "1900";

	if (mysql_query(con, query.c_str()) == 0) {
		cout << "\n    |         Brand         |  Unit  |" << endl;
		sql_res = mysql_store_result(con);
		while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
			if (strcmp(last_year, sql_row[0]) != 0) {
				strcpy_s(last_year, 10, sql_row[0]);
				cout << " " << sql_row[0] << " ---------------------------------" << endl;
				i = 1;
			}
			if (i > rank) continue;
			printf(" %2d | %21s | %6s |\n", i, sql_row[1], sql_row[2]);
			i++;
		}
		mysql_free_result(sql_res);
	}
}

void t6() {
	string query;
	int rank = 0;
	const char *months[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	cout << "\n------- TYPE 6 -------\n" << endl;
	cout << "** In what month(s) do convertibles sell best?" << endl;

	while (rank <= 0) {
		cout << " Until what rank?: ";
		cin >> rank;
	}

	query = string_format("\
		SELECT MONTH(ts_date), SUM(p)\n\
		FROM(\n\
			SELECT ts_date, price + ifnull(add_price, 0) as p\n\
			FROM vehicle left join model_spec on vehicle.M_ID = model_spec.M_ID\n\
			left join colors on vehicle.O_ID = colors.O_ID\n\
			WHERE ts_date is not null\n\
		)as t\n\
		GROUP BY month(ts_date)\n\
		ORDER BY SUM(p) DESC\n\
		limit %d", rank);

	int i = 1;

	if (mysql_query(con, query.c_str()) == 0) {
		cout << "\n    |    MONTH   |    SALES    |" << endl;
		cout << " --------------------------------" << endl;
		sql_res = mysql_store_result(con);
		while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
			printf(" %2d | %10s | %11s |\n", i, months[stoi(sql_row[0])-1], sql_row[1]);
			i++;
		}
		mysql_free_result(sql_res);
	}
}

void t7() {
	string query;
	int rank = 0;

	cout << "\n------- TYPE 7 -------\n" << endl;
	cout << "** Find those dealers who keep a vehicle in inventory for the longest average time. " << endl;

	while (rank <= 0) {
		cout << " Until what rank?: ";
		cin >> rank;
	}

	query = string_format("\
		SELECT D_name, id\n\
		FROM(\n\
			SELECT D_ID, AVG(inventory_date) as id\n\
			FROM(\n\
				SELECT DATEDIFF(ts_date, mf_date) as inventory_date, dealer.D_ID\n\
				FROM dealer left join vehicle on vehicle.D_ID = dealer.D_ID\n\
				WHERE mf_date is not null\n\
			) as t\n\
			GROUP BY D_ID\n\
		) as avg_d left join dealer on avg_d.D_ID = dealer.D_ID\n\
		order by id desc\n\
		limit %d", rank);
	
	int i = 1;

	if (mysql_query(con, query.c_str()) == 0) {
		cout << "\n    |    NAME    | AVG. INVENTORY DATE |" << endl;
		cout << " ---------------------------------------" << endl;
		sql_res = mysql_store_result(con);
		while ((sql_row = mysql_fetch_row(sql_res)) != NULL) {
			printf(" %2d | %10s | %14s days |\n", i, sql_row[0], sql_row[1]);
			i++;
		}
		mysql_free_result(sql_res);
	}
}

int main() {
	int choice = 1;

	time_t t = time(NULL);
	localtime_s(&today, &t);

	// init
	if (mysql_init(&mysql) == NULL)
		cout << "Init failed" << endl;

	// connect
	con = mysql_real_connect(&mysql, host, user, pw, db, 3306, (const char*)NULL, 0);

	if (con == NULL) {
		cout << "Connection failed (" << mysql_errno(&mysql) << "): " << mysql_error(&mysql) << endl;
		return 1;
	}

	cout << "MySQL Succesfully Connected.\n" << endl;

	// select project
	if (mysql_select_db(&mysql, db)) {
		cout << "Project selection failed (" << mysql_errno(&mysql) << "): " << mysql_error(&mysql) << endl;
		return 1;
	}

	// create table
	if (send_query_from_file("create.txt")) return 1;
	else cout << "Table created." << endl;

	// insert rows
	if (send_query_from_file("insert.txt")) return 1;
	else cout << "Rows inserted." << endl;

	cout << "\n\n------- Welcome! -------\n" << endl;
	cout << "** Due to insufficient amount of data, unexpected results may occur." << endl;

	while (choice) {
		cout << "\n------- SELECT QUERY TYPES -------\n" << endl;
		for (int i = 1; i <= 7; i++) cout << "\t" << i << ". TYPE " << i << endl;
		cout << "\t0.QUIT" << endl;
		
		cout << "Type: ";
		cin >> choice;
		
		switch(choice){
		case 1:
			t1(); break;
		case 2:
			t2(); break;
		case 3:
			t3(); break;
		case 4:
			t4(); break;
		case 5:
			t5(); break;
		case 6:
			t6(); break;
		case 7:
			t7(); break;
		}
	}

	// drop table
	if (send_query_from_file("drop.txt")) return 1;
	else cout << "\n\nTable Droped." << endl;
	
	mysql_close(&mysql);
}