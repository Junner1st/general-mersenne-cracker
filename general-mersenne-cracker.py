import sys

class MT19937:
    def __init__(self, state):
        self.state = list(state)
        self.index = 0

    def twist(self):
        for i in range(624):
            x = (self.state[i] & 0x80000000) | (self.state[(i + 1) % 624] & 0x7fffffff)
            xA = x >> 1
            if x & 1:
                xA ^= 0x9908b0df
            self.state[i] = self.state[(i + 397) % 624] ^ xA
        self.index = 0

    def extract_number(self):
        if self.index == 624:
            self.twist()
        y = self.state[self.index]
        y ^= y >> 11
        y ^= (y << 7) & 0x9d2c5680
        y ^= (y << 15) & 0xefc60000
        y ^= y >> 18
        self.index += 1
        return y & 0xffffffff

class BitMatrix:
    def __init__(self, n):
        self.n = n
        self.word_size = 64
        self.num_words = (n + self.word_size - 1) // self.word_size
        self.rows = [[0] * self.num_words for _ in range(n)]

        self.matrix_builder(n)

    def matrix_builder(self, n):
        # Build the matrix A
        for j in range(n):
            word_idx = j // 32
            bit_pos = j % 32
            state = [0] * 624
            state[word_idx] = 1 << bit_pos
            mt = MT19937(state)
            for i in range(n):
                y = mt.extract_number()
                msb = (y >> 31) & 1
                if msb:
                    self.set_bit(i, j)

    def set_bit(self, r, c):
        w = c // self.word_size
        b = c % self.word_size
        self.rows[r][w] |= (1 << b)

    def get_bit(self, r, c):
        w = c // self.word_size
        b = c % self.word_size
        return (self.rows[r][w] & (1 << b)) != 0

    def xor_row(self, r1, r2):
        for w in range(self.num_words):
            self.rows[r1][w] ^= self.rows[r2][w]

    def swap_row(self, r1, r2):
        self.rows[r1], self.rows[r2] = self.rows[r2], self.rows[r1]

class MT19937Cracker:
    def __init__(self):
        self.n = 19968
        self.__bitMatrix = BitMatrix(self.n)
        self._x = [0] * self.n
        self.state = [0] * 624
        self._state_recovered = False
        self._mt = None

    def gaussian_elimination(self, observation) -> int:
        # Gaussian elimination with partial pivoting
        current_row = 0
        pivot_col = [-1] * self.n
        for col in range(self.n):
            pivot_row = None
            for row in range(current_row, self.n):
                if self.__bitMatrix.get_bit(row, col):
                    pivot_row = row
                    break
            if pivot_row is None:
                continue
            self.__bitMatrix.swap_row(current_row, pivot_row)
            observation[current_row], observation[pivot_row] = observation[pivot_row], observation[current_row]
            pivot_col[current_row] = col
            for row in range(self.n):
                if row != current_row and self.__bitMatrix.get_bit(row, col):
                    self.__bitMatrix.xor_row(row, current_row)
                    observation[row] ^= observation[current_row]
            current_row += 1

        return current_row, pivot_col
    
    def back_substitution(self, current_row, pivot_col):
        # Back substitution, setting free variables to 0
        for i in range(current_row - 1, -1, -1):
            col = pivot_col[i]
            sum_val = observation[i]
            for j in range(col + 1, self.n):
                if self.__bitMatrix.get_bit(i, j):
                    sum_val ^= self._x[j]
            self._x[col] = sum_val
    
    def consistency_checker(self, current_row):
        # Check consistency
        for i in range(current_row, self.n):
            if observation[i] != 0:
                print("Singular matrix, inconsistent system")
                sys.exit(1)

    def reconstruct_state(self):
        # Reconstruct initial state
        for j in range(self.n):
            if self._x[j]:
                word_idx = j // 32
                bit_pos = j % 32
                self.state[word_idx] |= 1 << bit_pos

        return self.state
    
    def advance_to_current(self):
        # Advance the MT to after the outputs
        mt = MT19937(self.state)
        for _ in range(self.n):
            mt.extract_number()

        return mt
    
    def cracker(self, observation):
        assert len(observation) >= self.n, "not enough valid bits"
        current_row, pivot_col = self.gaussian_elimination(observation)

        self.back_substitution(current_row, pivot_col)

        self.consistency_checker(current_row)

        self.reconstruct_state()

        self._mt = self.advance_to_current()
        self._state_recovered = True

        return self._mt
    
    def getrandbits(self):
        if not self._state_recovered:
            print("State not recovered yet.")
            return
        return self._mt.extract_number()
        
    
    def getstate(self):
        if not self._state_recovered:
            print("State not recovered yet.")
            return
        print(self.state)
        
        
def main(observation):
    
    cracker = MT19937Cracker()
    cracker.cracker(observation)

    pred = cracker.getrandbits()
    print(f"\npred={pred}")


if __name__ == "__main__":
    from random import getrandbits
    observation = [getrandbits(1) for _ in range(19968)]
    print(f"real={getrandbits(32)}")

    main(observation)
