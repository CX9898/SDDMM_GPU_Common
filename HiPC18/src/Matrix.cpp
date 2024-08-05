#include <iostream>
#include <fstream>
#include <string>

#include "Matrix.hpp"

/**
 *
 **/
std::string iterateOneWord(const std::string &line, int &lineIter) {
    const int begin = lineIter;
    while (lineIter < line.size() && line[lineIter] != ' ') {
        ++lineIter;
    }
    const int end = lineIter;
    ++lineIter;

    return line.substr(begin, end - begin);
}

template<>
bool SparseMatrix<float>::initializeFromMatrixMarketFile(const std::string &filePath) {
    std::ifstream inFile;
    inFile.open(filePath, std::ios::in); // open file
    if (!inFile.is_open()) {
        std::cout << "Error,Matrix Market file cannot be opened." << std::endl;
        return false;
    }

    std::string line; // Store the data for each line
    getline(inFile, line); // First line does not operate

    getline(inFile, line);
    int lineIter = 0;
    _row = std::stoi(iterateOneWord(line, lineIter));
    _col = std::stoi(iterateOneWord(line, lineIter));
    _nnz = std::stoi(iterateOneWord(line, lineIter));

    if (lineIter < line.size()) {
        std::cerr << "Error, Matrix Market file " << line << " line format is incorrect!" << std::endl;
    }

    _rowIndex.resize(_nnz);
    _colIndex.resize(_nnz);
    _values.resize(_nnz);

    int idx = 0;
    while (getline(inFile, line)) {
        lineIter = 0;
        const int row = std::stoi(iterateOneWord(line, lineIter)) - 1;
        const int col = std::stoi(iterateOneWord(line, lineIter)) - 1;
        const float val = (float) std::stof(iterateOneWord(line, lineIter));

        if (lineIter < line.size()) {
            std::cerr << "Error, Matrix Market file " << line << " line format is incorrect!" << std::endl;
        }

        _rowIndex[idx] = row;
        _colIndex[idx] = col;
        _values[idx] = val;

        ++idx;
    }

    inFile.close();

    return true;
}

template<>
bool Matrix<float>::initializeFromSparseMatrix(const SparseMatrix<float> &matrixS) {
    _row = matrixS.row();
    _col = matrixS.col();
    const int size = matrixS.row() * matrixS.col();
    _size = size;
    _matrixOrder = MatrixOrder::row_major;
    const int ld = matrixS.col();
    _leadingDimension = ld;

    const auto &rowIndexS = matrixS.rowIndex();
    const auto &colIndexS = matrixS.colIndex();
    const auto &valuesS = matrixS.values();

    _values.clear();
    _values.resize(size);
    for (int idx = 0; idx < matrixS.nnz(); ++idx) {
        const int row = rowIndexS[idx];
        const int col = colIndexS[idx];
        const auto val = valuesS[idx];

        _values[row * ld + col] = val;
    }

    return true;
}

template<>
void Matrix<float>::changeMajorOrder() {
    const auto oldMajorOrder = _matrixOrder;
    const auto oldLd = _leadingDimension;
    const auto &oldValues = _values;

    MatrixOrder newMatrixOrder;
    size_t newLd;
    std::vector<float> newValues(_size);
    if (oldMajorOrder == MatrixOrder::row_major) {
        newMatrixOrder = MatrixOrder::col_major;
        newLd = _row;

        for (int idx = 0; idx < oldValues.size(); ++idx) {
            const int row = idx / oldLd;
            const int col = idx % oldLd;
            const auto val = oldValues[idx];

            newValues[col * newLd + row] = val;
        }
    } else if (oldMajorOrder == MatrixOrder::col_major) {
        newMatrixOrder = MatrixOrder::row_major;
        newLd = _col;

        for (int idx = 0; idx < _values.size(); ++idx) {
            const int col = idx / oldLd;
            const int row = idx % oldLd;
            const auto val = _values[idx];

            newValues[row * newLd + col] = val;
        }
    }

    _matrixOrder = newMatrixOrder;
    _leadingDimension = newLd;
    _values = newValues;
}