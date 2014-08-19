void func(float*restrict input, float*restrict output, int size_x, int size_y) {
  int i, j;
  for (i = 0; i < size_y; ++i) {
    for (j = 0; j < size_x; ++j) {
      output[i * size_x + j] = (
          input[(i - 1) * size_x + j - 1] +
          input[(i - 1) * size_x + j] + 
          input[(i - 1) * size_x + j + 1] +
          input[(i) * size_x + j - 1] +
          input[(i) * size_x + j] +
          input[(i) * size_x + j + 1] +
          input[(i + 1) * size_x + j - 1] +
          input[(i + 1) * size_x + j] +
          input[(i + 1) * size_x + j + 1]) * 0.11;
    }
  }
}

