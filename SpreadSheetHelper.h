//
// Created by andrei on 20.03.23.
//

#ifndef RCR_SPREADSHEETHELPER_H
#define RCR_SPREADSHEETHELPER_H


#include <string>
#include <vector>
#include <map>

#include "gen/rcr.pb.h"

class SheetRow {
public:
    int id;                                 // column A
    std::string name;                       // B
    std::vector <std::string> properties;   // B after ','
    int qty;                                // C
    std::string property_dip;               // D
    std::string remarks;                    // E

    SheetRow() = default;
    void toCardRequest(
        const std::string &operation,
        const std::string &componentSymbol,
        uint64_t box,
        rcr::CardRequest &retval
    ) const;

};

class SpreadSheetHelper {
public:
    explicit SpreadSheetHelper(
        const std::string &fileName
    );
    explicit SpreadSheetHelper(
        const std::string &fileName,
        const std::string &content
    );
    int loadFile(
        const std::string &fileName
    );
    int loadString(
        const std::string &content
    );

    // result
    std::vector <SheetRow> items;
    // statistics
    size_t total;
    std::map<int, size_t> boxItemCount;
};

#endif //RCR_SPREADSHEETHELPER_H
