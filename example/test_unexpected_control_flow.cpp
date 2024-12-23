void test() {
    int arr[]{1, 2, 3, 4, 5};
    lab:
      [[parallel]]
      for (auto &i : arr) {
          i = i * 2;
          break;
          continue;
          break;
          return;
          { goto lab; }
      }
    for (auto &i : arr) {
        i = i * 2;
        break;
        continue;
        break;
        return;
        { goto lab; }
    }
}