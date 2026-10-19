extern "C" int probe_eh(int n) {
    try {
        if (n) throw n;
        return 0;
    } catch (int x) {
        return x+10;
    }
}
