#pragma  once

#include <string>
#include <vector>

enum MatrixOrder {
  row_major,
  col_major
};

/**
 * SparseMatrix class
 *
 * Store in COO format.
 **/
template<typename T>
class SparseMatrix {
 public:
  SparseMatrix() = default;
  ~SparseMatrix() = default;

  /**
   * Initialize from MatrixMarket file.
   *
   * MatrixMarket file format:
   *    1) The first line describes the file format.
   *    2) The second line has three numbers separated by a space: number of rows, number of columns, and number of non-zeros.
   *    3) Each after line has three numbers separated by a space: current row, current column, and value.
   **/
  bool initializeFromMatrixMarketFile(const std::string &filePath);

  size_t nnz() const {
      return _nnz;
  }

  size_t row() const {
      return _row;
  }
  size_t col() const {
      return _col;
  }

  const std::vector<int> &rowIndex() const {
      return _rowIndex;
  }
  const std::vector<int> &colIndex() const {
      return _colIndex;
  }
  const std::vector<T> &values() const {
      return _values;
  }

 private:
  size_t _nnz;
  size_t _row;
  size_t _col;

  std::vector<int> _rowIndex;
  std::vector<int> _colIndex;
  std::vector<T> _values;
};

/**
 * The default is row-major order, but if you want to switch to column-major order, call the changeMajorOrder function.
 **/
template<typename T>
class Matrix {
 public:
  Matrix() = default;
  ~Matrix() = default;

  Matrix(size_t row,
         size_t col,
         size_t size,
         MatrixOrder matrixOrder,
         size_t leadingDimension,
         const std::vector<T> &values)
      : _row(row),
        _col(col),
        _size(size),
        _matrixOrder(matrixOrder),
        _leadingDimension(leadingDimension),
        _values(values) {}

  bool initializeFromSparseMatrix(const SparseMatrix<T> &matrixS);
  void changeMajorOrder();

  size_t size() const {
      return _size;
  }
  MatrixOrder matrixOrder() const {
      return _matrixOrder;
  }
  size_t ld() const {
      return _leadingDimension;
  }
  size_t row() const {
      return _row;
  }
  size_t col() const {
      return _col;
  }
  const std::vector<T> &values() const {
      return _values;
  }

  void setSize(size_t size) {
      _size = size;
      _values.resize(size);
  }
  void setld(size_t ld) {
      _leadingDimension = ld;
  }
  void setRow(size_t row) {
      _row = row;
  }
  void setCol(size_t col) {
      _row = col;
  }
  std::vector<T> &setValues() {
      return _values;
  }

 private:
  size_t _row;
  size_t _col;
  size_t _size;
  MatrixOrder _matrixOrder;
  size_t _leadingDimension;

  std::vector<T> _values;
};