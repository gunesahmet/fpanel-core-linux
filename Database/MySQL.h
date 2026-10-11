#pragma once
#ifndef CORE_MYSQL_H
#define CORE_MYSQL_H

#include <iostream>
#include <map>
#include <Helpers/Helpers.h>
#include <Logger/Logger.h>
#include <superior_mysqlpp/dnsa_connection_pool.hpp>
#include <superior_mysqlpp/shared_ptr_pool/sleep_in_parts.hpp>

/**
 * Usage:
    MySQL mysql(config.configurations["main.db_host"],
    config.configurations["main.db_port"],
    config.configurations["main.db_name"],
    config.configurations["main.db_username"],
    config.configurations["main.db_password"]);

    mysql.select({
        "domains.id as domain_id",
        "domains.name as domain_name",
        "domain_whois.registered_date as registered_date"
    })
    .from("domains")
    .innerJoin("domain_whois","domain_whois.domain_id = domains.id")
    .orderAsc("domain_name")
    .limit(1)
    .find();
    std::map<int, std::map<std::string,std::string>> result = mysql.result();

    for (std::map<int, std::map<std::string,std::string>> ::iterator it = result.begin(); it != result.end(); it++) {
        helpers.Print("Row: ");
        helpers.Println(it->first);
        helpers.Print("Column Key: ");
        helpers.Println(it->second["domain_id"]);
        helpers.Print("Column Value: ");
        helpers.Println(it->second["domain_name"]);
        helpers.Print("Column Value: ");
        helpers.Println(it->second["registered_date"]);
    }
 */

using namespace SuperiorMySqlpp;
using namespace std;
class MySQL {
private:

    SuperiorMySqlpp::Connection connection;

    Helpers helpers;
    Logger _logger;

    std::map<int,std::string> _and_where;
    std::map<int,std::string> _or_where;
    std::map<int,std::map<std::string,std::string>> _inner_join;
    std::map<int,std::map<std::string,std::string>> _left_join;
    std::map<int, std::map<std::string,std::string>> *_result;
    bool _result_status = false;
    std::string _inserted_id;

    std::map<int, std::string> _columns;

    // For Select Queries
    std::string _order_desc;
    std::string _order_asc;
    std::string _where;
    std::string _select;
    std::string _group_by;
    std::string _from;
    std::string _limit;
    bool _distinct;
    std::string _generated_select_sql;

    // For Insert Update Remove Queries
    std::string _generated_insert_sql;
    std::string _generated_update_sql;
    std::string _generated_remove_sql;
    bool is_connect;

public:

    MySQL(std::string host, int port, std::string dbname, std::string username, std::string password): connection(dbname, username, password, host, port)
    {
        _and_where = std::map<int,std::string>();
        _or_where = std::map<int,std::string>();
        _inner_join = std::map<int,std::map<std::string,std::string>>();
        _left_join = std::map<int,std::map<std::string,std::string>>();
        _columns = std::map<int, std::string>();
        _generated_select_sql = "";
        is_connect = connection.tryPing();
    }

    void SetLogger(Logger logger)
    {
        _logger = logger;
    }

    MySQL& select(std::vector<std::string> selects)
    {
        _result = new std::map<int, std::map<std::string,std::string>>();
        _result->clear();
        for(std::size_t i=0; i < selects.size(); ++i) {
            std::string col(selects[i].substr(selects[i].rfind(" as ") + 1));
            helpers.ReplaceWithString(col, "as ", "");
            _columns.insert({_columns.size(),col});
            _select.append(selects[i]);
            _select.append(",");
        }
        _select = _select.substr(0, _select.size()-1);
        return *this;
    }

    MySQL& distinct(bool distinct)
    {
        _distinct = distinct;
        return *this;
    }

    MySQL& from(const std::string &table)
    {

        _from = table;
        return *this;
    }

    MySQL& where(std::string condition, std::map<std::string, std::string> bind)
    {
        for (std::map<std::string, std::string>::iterator it = bind.begin(); it != bind.end(); it++) {
            std::string search = "?";
            search.append(it->first);
            std::string replace = it->second;
            helpers.ReplaceWithString(condition, search, replace);
        }

        _where = condition;
        return *this;
    }

    MySQL& andWhere(std::string condition, std::map<std::string, std::string> bind)
    {
        for (std::map<std::string, std::string>::iterator it = bind.begin(); it != bind.end(); it++) {
            std::string search = "?";
            search.append(it->first);
            std::string replace = it->second;
            helpers.ReplaceWithString(condition, search, replace);
        }
        _and_where.insert({_and_where.size(), condition});
        return *this;
    }

    MySQL& orWhere(std::string condition, std::map<std::string, std::string> bind)
    {
        for (std::map<std::string, std::string>::iterator it = bind.begin(); it != bind.end(); it++) {
            std::string search = "?";
            search.append(it->first);
            std::string replace = it->second;
            helpers.ReplaceWithString(condition, search, replace);
        }
        _or_where.insert({_or_where.size(), condition});
        return *this;
    }

    MySQL& limit(int limit)
    {
        _limit = " LIMIT " + std::to_string(limit);
        return *this;
    }

    MySQL& limitOffSet(int limit_off_set)
    {
        _limit.append(" OFFSET ");
        _limit.append(std::to_string(limit_off_set));
        return *this;
    }

    MySQL& orderDesc(const std::string &order)
    {
        _order_desc = order;
        return *this;
    }

    MySQL& orderAsc(const std::string &order)
    {
        _order_asc = order;
        return *this;
    }

    MySQL& innerJoin(const std::string &table, std::string relation)
    {
        std::map<std::string, std::string> join_data = std::map<std::string, std::string>();
        join_data.insert({"table", table});
        join_data.insert({"relation", relation});
        _inner_join.insert({_inner_join.size(), join_data});
        return *this;
    }

    MySQL& leftJoin(const std::string &table, std::string relation)
    {
        std::map<std::string, std::string> join_data = std::map<std::string, std::string>();
        join_data.insert({"table", table});
        join_data.insert({"relation", relation});
        _left_join.insert({_left_join.size(), join_data});
        return *this;
    }

    MySQL& groupBy(std::vector<std::string> group_by)
    {
        std::string _group_bys;
        for(std::size_t i=0; i<group_by.size(); ++i) {
            if(_group_by.empty()) {
                _group_bys = group_by[i];
            }
            else {
                _group_bys.append(",");
                _group_bys.append(group_by[i]);
            }
        }
        _group_by = " GROUP BY " + _group_bys;
        return *this;
    }

    MySQL& find()
    {
        _result_status = false;
        generateSql();

        try {
            Query query = connection.makeQuery(_generated_select_sql);
            query.execute();

            auto result = query.store();

            if(result.getRowsCount() > 0) {
                for(int i = 0; i < result.getRowsCount(); i++) {
                    auto row = result.fetchRow();
                    std::map<std::string,std::string> res = std::map<std::string,std::string>();
                    for (std::map<int, std::string>::iterator it = _columns.begin(); it != _columns.end(); it++) {
                        res.insert( {it->second, row[it->first].getStringView().to_string() });
                    }
                    _result->insert({ _result->size(), res});
                }

                _result_status = true;
            }
        }
        catch(std::exception& e) {
            std::cerr << e.what() << std::endl;
        }

        _generated_select_sql = "";
        return *this;
    }

    bool resultStatus()
    {
        return _result_status;
    }

    void clear()
    {
        _order_desc.clear();
        _order_asc.clear();
        _columns.clear();
        _where.clear();
        _and_where.clear();
        _or_where.clear();
        _group_by.clear();
        _from.clear();
        _inner_join.clear();
        _left_join.clear();
        _limit.clear();
        _select.clear();
        _generated_select_sql = "";
    }

    std::map<int, std::map<std::string,std::string>>* result()
    {
        clear();
        return _result;
    }

    std::string toSql()
    {
        return _generated_select_sql;
    }

    std::string toUpdateSql()
    {
        return _generated_update_sql;
    }

    MySQL& generateSql()
    {
        std::string inner_join;
        for (std::map<int,std::map<std::string, std::string>>::iterator it = _inner_join.begin(); it != _inner_join.end(); it++) {
            std::map<std::string,std::string> joins = it->second;
            inner_join.append(std::string(" INNER JOIN "))
                    .append(joins["table"])
                    .append(" ON ")
                    .append(joins["relation"]);
        }
        std::string left_join;
        for (std::map<int,std::map<std::string, std::string>>::iterator it = _left_join.begin(); it != _left_join.end(); it++) {
            std::map<std::string,std::string> joins = it->second;
            left_join.append(std::string(" LEFT JOIN "))
                    .append(joins["table"])
                    .append(" ON ")
                    .append(joins["relation"]);
        }
        std::string where;
        std::string and_where;
        std::string or_where;
        if (!_where.empty()) {
            where = std::string(" WHERE ");
            where.append(_where);
            for (std::map<int,std::string>::iterator it = _and_where.begin(); it != _and_where.end(); it++) {
                std::string wd = it->second;
                and_where.append(std::string(" AND ") + wd);
            }

            for (std::map<int,std::string>::iterator it = _or_where.begin(); it != _or_where.end(); it++) {
                std::string owd = it->second;
                or_where.append(std::string(" OR ") + owd);
            }
        }

        std::string order;
        if(!_order_desc.empty()) {
            order = " ORDER BY " + _order_desc + " DESC";
        }
        else if(!_order_asc.empty()) {
            order = " ORDER BY " + _order_asc + " ASC";
        }

        if(_distinct) {
            _generated_select_sql = "SELECT DISTINCT ";
        }
        else {
            _generated_select_sql = "SELECT ";
        }

        _generated_select_sql += _select;
        _generated_select_sql += " FROM ";
        _generated_select_sql += _from;
        _generated_select_sql += inner_join;
        _generated_select_sql += left_join;
        _generated_select_sql += " ";
        _generated_select_sql += where;
        _generated_select_sql += and_where;
        _generated_select_sql += or_where;
        _generated_select_sql += _group_by;
        _generated_select_sql += order;
        _generated_select_sql += _limit;
        _generated_select_sql += ";";
        return *this;
    }

// For Inserts
    MySQL& insertColumns(std::vector<std::string> columns)
    {
        std::string insert_columns;
        for(std::size_t i=0; i < columns.size(); ++i) {
            insert_columns.append(columns[i]);
            insert_columns.append(",");
        }
        insert_columns = insert_columns.substr(0, insert_columns.size()-1);

        _generated_insert_sql += " (";
        _generated_insert_sql +=  insert_columns;
        _generated_insert_sql +=  ") VALUES ";

        return *this;
    }

    MySQL& startValue()
    {
        _generated_insert_sql += "(";
        return *this;
    }

    MySQL& insertSetValue(const std::string &value)
    {
        _generated_insert_sql += "'";
        _generated_insert_sql += value;
        _generated_insert_sql += "',";

        return *this;
    }

    MySQL& insertSetValue(int value)
    {
        _generated_insert_sql += "'";
        _generated_insert_sql += std::to_string(value);
        _generated_insert_sql += "',";

        return *this;
    }

    MySQL& insertSetValue(double value)
    {
        _generated_insert_sql += "'";
        _generated_insert_sql += std::to_string(value);
        _generated_insert_sql += "',";
        return *this;
    }

    MySQL& endValue()
    {
        helpers.RemoveLastChar(_generated_insert_sql);
        _generated_insert_sql += "),";
        return *this;
    }

    MySQL& insert(const std::string &table)
    {
        // INSERT INTO table_name (column_name_1, column_name_2, column_name_3, ..., column_name_n) VALUES (value_1, value_2, value_3, ..., value_n)
        _generated_insert_sql = "INSERT INTO ";
        _generated_insert_sql += table;
        return *this;
    }

    std::string toInsertSql()
    {
        return _generated_insert_sql;
    }

    bool executeInsert()
    {
        _result_status = false;
        helpers.RemoveLastChar(_generated_insert_sql);
        _generated_insert_sql += ";";
        _logger.Debug("Insert Query: " + _generated_insert_sql);
        try {
            Query query = connection.makeQuery(_generated_insert_sql);
            query.execute();
//            std::cout << "Inserted ID: " << query.getInsertId() << std::endl;
            _inserted_id = std::to_string(query.getInsertId());
            _generated_insert_sql = "";
            _result_status = true;
            clear();
            return true;
        }
        catch(std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
        clear();
        _generated_insert_sql = "";
        return false;
    }

// For Updates
    MySQL& update(const std::string &table)
    {
        // UPDATE table_name SET where_column_name = 'where_column_value' WHERE where_column_name = 'value'
        _generated_update_sql = "UPDATE ";
        _generated_update_sql += table;
        _generated_update_sql += " SET ";
        return *this;
    }

    MySQL& updateSetColumns(std::map<std::string,std::string> columns_and_values)
    {
        for (std::map<std::string, std::string>::iterator it = columns_and_values.begin(); it != columns_and_values.end(); it++) {
            _generated_update_sql.append(it->first)
                    .append(" = ")
                    .append("'")
                    .append(it->second)
                    .append("',");
        }
        _generated_update_sql = _generated_update_sql.substr(0, _generated_update_sql.size()-1);
        _generated_update_sql.append(" ");
        return *this;
    }

    MySQL& updateWhere(std::string condition, std::map<std::string, std::string> bind)
    {
        _generated_update_sql.append(" WHERE ");
        for (std::map<std::string, std::string>::iterator it = bind.begin(); it != bind.end(); it++) {
            std::string search = "?"+it->first;
            std::string replace = it->second;
            helpers.ReplaceWithString(condition, search, replace);
        }
        _generated_update_sql.append(condition);
        return *this;
    }

    bool executeUpdate()
    {
        _result_status = false;
        _generated_update_sql.append(";");

        try {
            Query query = connection.makeQuery(_generated_update_sql);
            query.execute();
            _generated_update_sql = "";
            _result_status = true;
            clear();
            return true;
        }
        catch(std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
        _generated_update_sql = "";
        clear();
        return false;
    }

// For Removes
    MySQL& remove(const std::string &table)
    {
        //DELETE FROM table_name WHERE column_name = column_value;
        _generated_remove_sql.append("DELETE FROM ")
                .append(table);
        return *this;
    }

    MySQL& removeWhere(std::string condition, std::map<std::string, std::string> bind)
    {
        _generated_remove_sql.append(" WHERE ");
        for (std::map<std::string, std::string>::iterator it = bind.begin(); it != bind.end(); it++) {
            std::string search = "?"+it->first;
            std::string replace = it->second;
            helpers.ReplaceWithString(condition, search.c_str(), replace.c_str());
        }
        _generated_remove_sql.append(condition);
        return *this;
    }

    bool executeRemove()
    {
        _result_status = false;
        _generated_remove_sql.append(";");

        try {
            Query query = connection.makeQuery(_generated_remove_sql);
            query.execute();
            _generated_remove_sql = "";
            _result_status = true;
            clear();
            return true;
        }
        catch(std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
        _generated_remove_sql = "";
        clear();
        return false;
    }

    void Close()
    {

    }

    bool isConnect()
    {
        return is_connect;
    }

    std::string InsertedId()
    {
        return _inserted_id;
    }
};


#endif //CORE_MYSQL_H
