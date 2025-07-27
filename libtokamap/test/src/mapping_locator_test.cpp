#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "utils/mapping_locator.hpp"
#include "utils/types.hpp"

using namespace libtokamap;

TEST_CASE("Test selection of mapping directories", "[mapping_locator]")
{
    SECTION("select exact string match")
    {
        std::vector<std::string> subdirectories = {"dir1", "dir2", "dir3"};
        auto selected = detail::select_subdirectory(subdirectories, DirectorySelector::Exact, "dir2");
        REQUIRE(selected == "dir2");
    }

    SECTION("failing to find exact match throws exception")
    {
        std::vector<std::string> subdirectories = {"dir1", "dir2", "dir3"};
        REQUIRE_THROWS(detail::select_subdirectory(subdirectories, DirectorySelector::Closest, "dir4"));
    }

    SECTION("other selectors raise exception for string values")
    {
        std::vector<std::string> subdirectories = {"dir1", "dir2", "dir3"};
        REQUIRE_THROWS(detail::select_subdirectory(subdirectories, DirectorySelector::Closest, "dir"));
        REQUIRE_THROWS(detail::select_subdirectory(subdirectories, DirectorySelector::MaxBelow, "dir"));
        REQUIRE_THROWS(detail::select_subdirectory(subdirectories, DirectorySelector::MinAbove, "dir"));
    }

    SECTION("select closest value directory")
    {
        std::vector<std::string> subdirectories = {"0", "100", "1000"};
        auto selected = detail::select_subdirectory(subdirectories, DirectorySelector::Closest, 200);
        REQUIRE(selected == "100");
    }

    SECTION("select largest value below directory")
    {
        std::vector<std::string> subdirectories = {"0", "100", "1000"};
        auto selected = detail::select_subdirectory(subdirectories, DirectorySelector::MaxBelow, 200);
        REQUIRE(selected == "100");
    }

    SECTION("value smaller than all directories throws exception")
    {
        std::vector<std::string> subdirectories = {"100", "1000"};
        REQUIRE_THROWS(detail::select_subdirectory(subdirectories, DirectorySelector::MaxBelow, 50));
    }

    SECTION("select smallest value above directory")
    {
        std::vector<std::string> subdirectories = {"0", "100", "1000"};
        auto selected = detail::select_subdirectory(subdirectories, DirectorySelector::MinAbove, 200);
        REQUIRE(selected == "1000");
    }

    SECTION("value larger than all directories throws exception")
    {
        std::vector<std::string> subdirectories = {"0", "100", "1000"};
        REQUIRE_THROWS(detail::select_subdirectory(subdirectories, DirectorySelector::MinAbove, 2000));
    }
}
