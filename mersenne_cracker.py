import mt19937
import bitmatrix
import mt19937cracker


class MT19937:
    def __init__(self, state):
        self._c_mt = mt19937.MT19937(state)

    def extract_number(self):
        return self._c_mt.extract_number()

class BitMatrix:
    def __init__(self, n):
        self.n = n
        self._c_bm = bitmatrix.BitMatrix(self.n)

    def get_bit(self, r, c):
        return self._c_bm.get_bit(r, c)

    def xor_row(self, r1, r2):
        return self._c_bm.xor_row(r1, r2)

    def swap_row(self, r1, r2):
        return self._c_bm.swap_row(r1, r2)


class MT19937Cracker:
    def __init__(self):
        self._c_mc = mt19937cracker.MT19937Cracker()

    def cracker(self, obs, bits: int) -> None:
        self._c_mc.cracker(obs, bits)

    def getrandbits(self) -> int:
        return self._c_mc.getrandbits()

    def getstate(self) -> list:
        return self._c_mc.getstate()


def get_obs_data(bits):
    observation_str = ""
    valid_bits = 1 << (bits.bit_length() -1)
    print(f"valid_bits={valid_bits}")
    observe_number = (19968+valid_bits-1) // valid_bits

    for _ in range(observe_number):
        rand_str = f"{getrandbits(bits)>>(bits-valid_bits):0{valid_bits}b}"
        observation_str += rand_str

    return list(map(int, list(observation_str)))[-19968:], valid_bits

def main(observation, bits):
    cracker = MT19937Cracker()
    cracker.cracker(observation, bits)

    pred = cracker.getrandbits()
    print(f"\npred={pred}")

if __name__ == "__main__":
    from random import getrandbits, seed
    k = 19
    observation_list, valid_bits = get_obs_data(k)

    print(f"real={getrandbits(32)}")

    main(observation_list, valid_bits)
