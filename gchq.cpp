#include <boost/logic/tribool.hpp>
#include <boost/multi_array.hpp>
#include <cstring>
#include <list>
#include <vector>
#include <set>

class partition_generator {
public:
    partition_generator(size_t n, size_t k):
        m_k(k),
        m_partitions(n-1) {
    }

    std::vector<size_t> get() {
        size_t s = m_partitions.size();
        std::vector<size_t> partitions;
        partitions.reserve(s + 1);
        partitions.push_back(*m_partitions.rbegin());
        for (size_t i = 1; i < s; ++i)
            partitions.push_back(m_partitions[s - i - 1] - m_partitions[s - i]);
        partitions.push_back(m_k - *m_partitions.begin());
        return partitions;
    }

    bool next() {
        for (size_t i = 0; i < m_partitions.size(); ++i) {
            if (m_partitions[i] < m_k) {
                std::fill(m_partitions.begin(), m_partitions.begin() + i + 1, m_partitions[i] + 1);
                return true;
            }
        }
        return false;
    }

private:
    size_t m_k;
    std::vector<size_t> m_partitions;
};

class board {
public:
    typedef boost::multi_array<boost::logic::tribool, 2> field;

    board(const std::vector<std::vector<size_t>>& rows,
          const std::vector<std::vector<size_t>>& columns,
          const std::set<std::pair<field::index, field::index>>& given):
        m_rows(rows),
        m_columns(columns),
        m_row_possibilities(m_rows.size()),
        m_column_possibilities(m_columns.size()),
        m_field(boost::extents[m_rows.size()][m_columns.size()]),
        m_num_cells_solved(given.size()) {
        std::fill(m_field.data(), m_field.data() + m_field.num_elements(), boost::logic::indeterminate);
        std::for_each(given.begin(), given.end(), [this](const auto& c){ m_field[c.first][c.second] = true; });
    }

    const field& solve() {
        generate_possibilities<false>();
        generate_possibilities<true>();
        while (m_num_cells_solved != m_field.num_elements()) {
            eliminate_impossible<false>();
            eliminate_impossible<true>();
            find_forced_cells<false>();
            find_forced_cells<true>();
        }
        return m_field;
    }

private:
    template <bool generate_rows>
    void generate_possibilities() {
        auto& primary = generate_rows ? m_rows : m_columns;
        auto& secondary = generate_rows ? m_columns : m_rows;
        auto& possibilities = generate_rows ? m_row_possibilities : m_column_possibilities;
        for (size_t index = 0; index < primary.size(); ++index) {
            std::vector<size_t>& runs = primary[index];
            size_t k = secondary.size() - (std::accumulate(runs.begin(), runs.end(), 0) + runs.size() - 1);
            partition_generator gen(runs.size() + 1, k);
            do {
                auto spaces = gen.get();
                std::transform(spaces.begin() + 1, spaces.end() - 1, spaces.begin() + 1, [](auto x){ return x + 1;});
                std::vector<bool> current(secondary.size());
                for (size_t j = 0, index = 0; j < runs.size() + spaces.size(); ++j) {
                    size_t length = (j % 2 == 0) ? spaces[j / 2] : runs[j / 2];
                    bool value = (j % 2 == 0) ? false : true;
                    std::fill(current.begin() + index, current.begin() + index + length, value);
                    index += length;
                }
                possibilities[index].push_back(current);
            } while (gen.next());
        }
    }

    template <bool prune_rows>
    void eliminate_impossible() {
        auto& possibilities = prune_rows ? m_row_possibilities : m_column_possibilities;
        for (size_t i = 0; i < possibilities.size(); ++i) {
            auto possibility_iter = possibilities[i].begin();
            while (possibility_iter != possibilities[i].end()) {
                bool changed = false;
                for (size_t j = 0; j < possibility_iter->size(); ++j) {
                    if ((prune_rows && (*possibility_iter)[j] != m_field[i][j]) ||
                        (!prune_rows && (*possibility_iter)[j] != m_field[j][i])) {
                        changed = true;
                        auto next_possibility = std::next(possibility_iter);
                        possibilities[i].erase(possibility_iter);
                        possibility_iter = next_possibility;
                        break;
                    }
                }
                if (!changed)
                    possibility_iter++;
            }
        }
    }

    template <bool find_rows>
    void find_forced_cells() {
        auto& primary = find_rows ? m_rows : m_columns;
        auto& secondary = find_rows ? m_columns : m_rows;
        auto& possibilities = find_rows ? m_row_possibilities : m_column_possibilities;
        for (size_t i = 0; i < primary.size(); ++i) {
            for (size_t j = 0; j < secondary.size(); ++j) {
                if (( find_rows && !boost::logic::indeterminate(m_field[i][j])) ||
                    (!find_rows && !boost::logic::indeterminate(m_field[j][i])))
                    continue;
                bool forced = true;
                bool val = (*possibilities[i].begin())[j];
                for (auto k = std::next(possibilities[i].begin()); k != possibilities[i].end(); ++k) {
                    if ((*k++)[j] != val) {
                        forced = false;
                        break;
                    }
                }
                if (forced) {
                    m_field[find_rows ? i : j][find_rows ? j : i] = val;
                    ++m_num_cells_solved;
                }
            }
        }
    }

    std::vector<std::vector<size_t>> m_rows;
    std::vector<std::vector<size_t>> m_columns;
    std::vector<std::list<std::vector<bool>>> m_row_possibilities;
    std::vector<std::list<std::vector<bool>>> m_column_possibilities;
    field m_field;
    size_t m_num_cells_solved;
};

int main() {
    static const std::vector<std::vector<size_t>> rows{ {            7, 3, 1,  1, 7},
                                                        {         1, 1, 2, 2,  1, 1},
                                                        {   1, 3, 1, 3, 1, 1,  3, 1},
                                                        {   1, 3, 1, 1, 6, 1,  3, 1},
                                                        {   1, 3, 1, 5, 2, 1,  3, 1},
                                                        {            1, 1, 2,  1, 1},
                                                        {      7, 1, 1, 1, 1,  1, 7},
                                                        {                      3, 3},
                                                        {1, 2, 3, 1, 1, 3, 1,  1, 2},
                                                        {         1, 2, 3, 2,  1, 1},
                                                        {         4, 1, 4, 2,  1, 2},
                                                        {   1, 1, 1, 1, 1, 4,  1, 3},
                                                        {         2, 1, 1, 1,  2, 5},
                                                        {         3, 2, 2, 6,  3, 1},
                                                        {         1, 9, 1, 1,  2, 1},
                                                        {         2, 1, 2, 2,  3, 1},
                                                        {      3, 1, 1, 1, 1,  5, 1},
                                                        {               1, 2,  2, 5},
                                                        {      7, 1, 2, 1, 1,  1, 3},
                                                        {      1, 1, 2, 1, 2,  2, 1},
                                                        {         1, 3, 1, 4,  5, 1},
                                                        {         1, 3, 1, 3, 10, 2},
                                                        {         1, 3, 1, 1,  6, 6},
                                                        {         1, 1, 2, 1,  1, 2},
                                                        {            7, 2, 1,  2, 5} };
    static const std::vector<std::vector<size_t>> columns{ {            7, 2, 1, 1, 7},
                                                           {         1, 1, 2, 2, 1, 1},
                                                           {1, 3, 1, 3, 1, 3, 1, 3, 1},
                                                           {   1, 3, 1, 1, 5, 1, 3, 1},
                                                           {   1, 3, 1, 1, 4, 1, 3, 1},
                                                           {         1, 1, 1, 2, 1, 1},
                                                           {      7, 1, 1, 1, 1, 1, 7},
                                                           {                  1, 1, 3},
                                                           {      2, 1, 2, 1, 8, 2, 1},
                                                           {   2, 2, 1, 2, 1, 1, 1, 2},
                                                           {            1, 7, 3, 2, 1},
                                                           {   1, 2, 3, 1, 1, 1, 1, 1},
                                                           {            4, 1, 1, 2, 6},
                                                           {      3, 3, 1, 1, 1, 3, 1},
                                                           {            1, 2, 5, 2, 2},
                                                           {2, 2, 1, 1, 1, 1, 1, 2, 1},
                                                           {      1, 3, 3, 2, 1, 8, 1},
                                                           {                  6, 2, 1},
                                                           {         7, 1, 4, 1, 1, 3},
                                                           {            1, 1, 1, 1, 4},
                                                           {         1, 3, 1, 3, 7, 1},
                                                           {1, 3, 1, 1, 1, 2, 1, 1, 4},
                                                           {         1, 3, 1, 4, 3, 3},
                                                           {      1, 1, 2, 2, 2, 6, 1},
                                                           {         7, 1, 3, 2, 1, 1} };
    static const std::set<std::pair<board::field::index, board::field::index>> given{ { 3,  3},
                                                                                      { 3,  4},
                                                                                      { 3, 12},
                                                                                      { 3, 13},
                                                                                      { 3, 21},
                                                                                      { 8,  6},
                                                                                      { 8,  7},
                                                                                      { 8, 10},
                                                                                      { 8, 14},
                                                                                      { 8, 15},
                                                                                      { 8, 18},
                                                                                      {16,  6},
                                                                                      {16, 11},
                                                                                      {16, 16},
                                                                                      {16, 20},
                                                                                      {21,  3},
                                                                                      {21,  4},
                                                                                      {21,  9},
                                                                                      {21, 10},
                                                                                      {21, 15},
                                                                                      {21, 20},
                                                                                      {21, 21} };
    board::field solution = board(rows, columns, given).solve();

    size_t width = solution.shape()[1];
    size_t height = solution.shape()[0];
    size_t image_bytes = 4 * width * height;
    unsigned char* img = new unsigned char[image_bytes];
    memset(img, 0, image_bytes);
    int filesize = 54 + image_bytes;
    for (size_t row = 0; row < solution.shape()[0]; ++row) {
        for (size_t column = 0; column < solution.shape()[1]; ++column) {
            img[(row * width + column) * 3 + 0] = solution[row][column] ? 0 : 255;
            img[(row * width + column) * 3 + 1] = solution[row][column] ? 0 : 255;
            img[(row * width + column) * 3 + 2] = solution[row][column] ? 0 : 255;
        }
    }
    unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
    unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
    unsigned char bmppad[3] = {0,0,0};
    bmpfileheader[ 2] = (unsigned char)(filesize >>  0);
    bmpfileheader[ 3] = (unsigned char)(filesize >>  8);
    bmpfileheader[ 4] = (unsigned char)(filesize >> 16);
    bmpfileheader[ 5] = (unsigned char)(filesize >> 24);
    bmpinfoheader[ 4] = (unsigned char)(width    >>  0);
    bmpinfoheader[ 5] = (unsigned char)(width    >>  8);
    bmpinfoheader[ 6] = (unsigned char)(width    >> 16);
    bmpinfoheader[ 7] = (unsigned char)(width    >> 24);
    bmpinfoheader[ 8] = (unsigned char)(height   >>  0);
    bmpinfoheader[ 9] = (unsigned char)(height   >>  8);
    bmpinfoheader[10] = (unsigned char)(height   >> 16);
    bmpinfoheader[11] = (unsigned char)(height   >> 24);
    FILE* f = fopen("img.bmp","wb");
    fwrite(bmpfileheader,1,14,f);
    fwrite(bmpinfoheader,1,40,f);
    for(size_t i = 0; i < height; ++i)
    {
        fwrite(img+(width*(height-i-1)*3),3,width,f);
        fwrite(bmppad,1,(4-(width*3)%4)%4,f);
    }
    fclose(f);
    return 0;
}
