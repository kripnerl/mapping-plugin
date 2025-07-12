#include "map_arguments.hpp"

#include <string>
#include <string_view>
#include <boost/algorithm/string.hpp>

json_mapping::SignalType json_mapping::deduce_signal_type(std::string_view final_path_element)
{
    // SignalType useful in determining for MAST-U
    SignalType sig_type{SignalType::DEFAULT};
    if (final_path_element.empty()) {
        sig_type = SignalType::INVALID;
    } else if (final_path_element == "data") {
        sig_type = SignalType::DATA;
    } else if (final_path_element == "time") {
        sig_type = SignalType::TIME;
    } else if (final_path_element.find("error") != std::string::npos) {
        sig_type = SignalType::ERROR;
    }
    return sig_type;
}
