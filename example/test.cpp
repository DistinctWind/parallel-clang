void test_traditional() {
  [[parallel]]
  for (int i = 1; i < 10; i++) {}
  [[parallel]]
  int x;
}

void test() {
  int arr[]{1, 2, 3, 4, 5};
  [[parallel]]
  for (auto &i : arr) {
    i = i * 2;
  }
  for (auto &i : arr) {
    i = i * 2;
  }
}