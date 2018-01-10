int func(int x) {
  write(x);
  return x * x;
}
int main () {
  int y, n;
  y = read();
  n = func(y);
  write(n);
  return 0;
}