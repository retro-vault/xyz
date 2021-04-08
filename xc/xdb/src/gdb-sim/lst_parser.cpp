#include <string>
#include <fstream>
#include <iostream>

/*

    Final state machine:
    


*/

int main() {
    std::string fname="../xyz/yos/build/console.lst";
    std::ifstream file;
    file.open(fname,std::ios::in);

    if (file.is_open()) {
        std::string line;
        int line_no=1;
        while (std::getline(file, line)) {
            
            // Ignore these!
            if (
                line.rfind("\fAS", 0)==0 // Page break 1/2 
                || line.rfind("Hex", 0)==0 // Page break 2/2
                || line.empty()
            )
                std::cout << line_no << " " << line << std::endl;

            // Increase line counter.
            line_no++;
        }
        file.close();
    }

    return 0;
}