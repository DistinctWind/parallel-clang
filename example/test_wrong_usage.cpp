void test_wrong_usage() {
  [[parallel]]
  for (int i = 1; i < 10; i++) {}
  [[parallel]]
  while (true) {}
  [[parallel]]
  int x;
}