import sys

import mt19937
import bitmatrix

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
        self.n = 19968
        self._x = [0] * self.n
        self.state = [0] * 624
        self._state_recovered = False
        self._mt = None

    def gaussian_elimination(self, observation, bitMatrix: BitMatrix) -> tuple:
        # Gaussian elimination with partial pivoting
        current_row = 0
        pivot_col = [-1] * self.n
        for col in range(self.n):
            pivot_row = None
            for row in range(current_row, self.n):
                if bitMatrix.get_bit(row, col):
                    pivot_row = row
                    break
            if pivot_row is None:
                continue
            bitMatrix.swap_row(current_row, pivot_row)
            observation[current_row], observation[pivot_row] = observation[pivot_row], observation[current_row]
            pivot_col[current_row] = col
            for row in range(self.n):
                if row != current_row and bitMatrix.get_bit(row, col):
                    bitMatrix.xor_row(row, current_row)
                    observation[row] ^= observation[current_row]
            current_row += 1

        return current_row, pivot_col, bitMatrix
    
    def back_substitution(self, current_row, pivot_col, bitMatrix: BitMatrix):
        # Back substitution, setting free variables to 0
        for i in range(current_row - 1, -1, -1):
            col = pivot_col[i]
            sum_val = observation[i]
            for j in range(col + 1, self.n):
                if bitMatrix.get_bit(i, j):
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
        current_row, pivot_col, bitMatrix = self.gaussian_elimination(observation, BitMatrix(self.n))

        self.back_substitution(current_row, pivot_col, bitMatrix)

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