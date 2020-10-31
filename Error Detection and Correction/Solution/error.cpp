#include <bits/stdc++.h>
#include "colors.h"

using namespace std;

class Block
{
  public:
	char **ara;
	int row, col, size;

	Block(char **ara, int row, int col)
	{
		this->ara = const_cast<char **>(ara);
		this->row = row;
		this->col = col;
		this->size = row * col;
	}
};

bool isPowerOfTwo(int num)
{
	if (num <= 0)
		return 0;
	return !(num & (num - 1));
}

string deciToBinary(int n)
{
	string binary;

	int i = 0;
	while (n > 0)
	{

		binary += (n % 2) + '0';
		n = n / 2;
		i++;
	}
	for (int i = binary.length(); i < 8; i++)
		binary += '0';
	reverse(binary.begin(), binary.end());
	return binary;
}

int logFloor(int num)
{
	int k = 0;
	while ((1 << k) <= num)
		k++;
	return k;
}

char binaryToDeci(string str)
{
	int res = 0;
	int bse = 1;
	reverse(str.begin(), str.end());
	for (int i = 0; i < str.length(); i++)
	{
		res += (str[i] - '0') * bse;
		bse *= 2;
	}
	return res;
}

string trimString(string &tmpStr)
{
	for (int i = 0; i < tmpStr.length(); i++)
		if (tmpStr[i] != '0')
			return tmpStr = tmpStr.substr(i);
	return tmpStr = "";
}

string xorString(string s1, string s2)
{
	string retStr;
	for (int i = 0; i < s1.length(); i++)
	{
		retStr += ((s1[i] - '0') ^ (s2[i] - '0')) + '0';
	}
	return retStr;
}
class Data
{

	string str;

  public:
	int m;
	double p;
	string gen_poly;

	void takeInput()
	{
		ifstream input;
		input.open("input.txt");
		cout << "Enter data string :";
		getline(input, str);
		cout << "Enter number of data bytes in a dataRow (m):";
		input >> m;
		cout << "Enter Probability (p):";
		input >> p;
		cout << "Enter generating polynomial:";
		input >> gen_poly;
		input.close();
	}

	string padString()
	{
		if (str.length() % m != 0)
		{
			int rem = m - str.length() % m;
			for (int i = 0; i < rem; i++)
				str += "~";
		}
		return str;
	}

	void printString()
	{
		cout << str << endl;
	}

	Block buildDataBlock()
	{
		int dataRow = str.length() / m;
		int dataCol = m * 8;
		int indx = 0;
		char **dataBlock = new char *[dataRow];
		for (int i = 0; i < dataRow; i++)
		{
			dataBlock[i] = new char[dataCol];

			for (int j = 0; j < m; j++)
			{
				string bin = deciToBinary(str[indx++]);
				//cout << bin << endl;
				for (int k = 0; k < 8; k++)
				{
					dataBlock[i][j * 8 + k] = bin[k];
				}
			}
		}

		return Block(dataBlock, dataRow, dataCol);
	}

	void printBlock(Block b)
	{
		int row = b.row;
		int col = b.col;
		char **ara = const_cast<char **>(b.ara);
		for (int i = 0; i < row; i++)
		{
			for (int j = 0; j < col; j++)
			{
				cout << ara[i][j];
			}
			cout << endl;
		}
		cout << "----------------------" << endl;
	}

	Block buildBlockWithParity(Block datablock)
	{
		int parityRow = datablock.row;
		int parityCol = datablock.col + logFloor(datablock.col);
		char **block = new char *[parityRow];
		for (int i = 0; i < parityRow; i++)
		{
			block[i] = new char[parityCol];

			int indx = 0;
			for (int c = 0; c < parityCol; c++)
			{
				if (isPowerOfTwo(c + 1))
				{
					block[i][c] = '+';
					continue;
				}
				block[i][c] = datablock.ara[i][indx++];
			}
			int k = 1;
			while (k < parityCol)
			{
				block[i][k - 1] = '0';
				int xorRes = 0;
				for (int j = k - 1; j < parityCol; j += 2 * k)
				{
					for (int m = 0; m < k; m++)
					{
						if (j + m == parityCol)
							break;
						xorRes = xorRes ^ (block[i][j + m] - '0');
					}
				}
				block[i][k - 1] = xorRes + '0';
				k *= 2;
			}
		}
		for (int i = 0; i < parityRow; i++)
		{
			for (int j = 0; j < parityCol; j++)
			{
				if (isPowerOfTwo(j + 1))
				{
					cout << FGRN(block[i][j]);
				}
				else
				{
					cout << block[i][j];
				}
			}
			cout << endl;
		}
		return Block(block, parityRow, parityCol);
	}

	Block serializeColWise(Block parityBlock)
	{
		char **block = new char *[1];
		block[0] = new char[parityBlock.size];
		int indx = 0;
		for (int i = 0; i < parityBlock.col; i++)
		{
			for (int j = 0; j < parityBlock.row; j++)
			{
				block[0][indx++] = parityBlock.ara[j][i];
			}
		}

		return Block(block, 1, parityBlock.size);
	}

	string appendCRCcheckSum(Block block)
	{
		string tmpStr(block.ara[0], block.size);
		int len = gen_poly.length();
		for (int i = 0; i < len - 1; i++)
			tmpStr += '0';
		while (tmpStr.length() >= gen_poly.length())
		{
			string xorStr = xorString(tmpStr.substr(0, len), gen_poly);
			tmpStr.replace(0, len, xorStr);

			trimString(tmpStr);
		}

		string retStr(block.ara[0], block.size);
		cout << retStr;
		for (int i = 0; i < (len - 1 - tmpStr.length()); i++)
		{
			retStr += '0';
			cout << FBLU("0");
		}
		retStr += tmpStr;
		cout << FBLU(tmpStr) << endl;
		return retStr;
	}

	bool computeCheckSum(string recvdStr)
	{
		int len = gen_poly.length();
		while (recvdStr.length() >= gen_poly.length())
		{
			string xorStr = xorString(recvdStr.substr(0, len), gen_poly);
			recvdStr.replace(0, len, xorStr);
			trimString(recvdStr);
		}
		return recvdStr.length() != 0;
	}

	Block buildCRCfreeBlock(string recvStr, bool isFlipped[])
	{
		string tmpStr = recvStr.substr(0, recvStr.length() - (gen_poly.length() - 1));
		int col = m * 8 + logFloor(m * 8);
		int row = tmpStr.length() / col;
		char **block = new char *[row];
		for (int i = 0; i < row; i++)
		{
			block[i] = new char[col];
		}
		int indx = 0;
		for (int c = 0; c < col; c++)
		{
			for (int r = 0; r < row; r++)
			{
				block[r][c] = tmpStr[indx++];
			}
		}
		for (int r = 0; r < row; r++)
		{
			for (int c = 0; c < col; c++)
			{
				if (isFlipped[c * row + r])
				{
					cout << FRED(block[r][c]);
				}
				else
					cout << block[r][c];
			}
			cout << endl;
		}
		return Block(block, row, col);
	}

	Block buildParityFreeBlock(Block parityBlock)
	{
		int row = parityBlock.row;
		int col = m * 8;
		char **block = new char *[row];

		for (int i = 0; i < row; i++)
		{
			block[i] = new char[col];
			int indx = 0;
			int k = 1;
			while (k < parityBlock.col)
			{
				int xorRes = 0;
				for (int j = k - 1; j < parityBlock.col; j += 2 * k)
				{
					for (int m = 0; m < k; m++)
					{
						if (j + m == parityBlock.col)
							break;
						xorRes = xorRes ^ (parityBlock.ara[i][j + m] - '0');
					}
				}
				if (xorRes != 0)
				{ // ok
					indx += k;
				}
				k *= 2;
			}
			// problem in indx-th index . flip that bit
			if (indx != 0)
			{
				parityBlock.ara[i][indx - 1] = parityBlock.ara[i][indx - 1] == '0' ? '1' : '0';
			}

			indx = 0;

			for (int j = 0; j < parityBlock.col; j++)
			{
				if (isPowerOfTwo(j + 1))
					continue;
				block[i][indx++] = parityBlock.ara[i][j];
			}
		}
		return Block(block, row, col);
	}

	string decodeBlock(Block block)
	{
		string res;
		int bits = 0;
		for (int i = 0; i < block.row; i++)
		{
			for (int j = 0; j < block.col; j += 8, bits++)
			{
				char ch = binaryToDeci(string(block.ara[i] + j, block.ara[i] + j + 8));
				res += ch;
			}
		}
		return res;
	}
};

double randGen()
{
	const int range_from = 0;
	const int range_to = RAND_MAX;
	random_device rand_dev;
	mt19937 generator(rand_dev());
	uniform_int_distribution<int> distr(range_from, range_to);

	return distr(generator);
}
int main()
{

	Data d;
	d.takeInput();
	cout << "data string after padding: ";
	d.padString();
	d.printString();
	cout << "data block (ascii code of m characters per row):" << endl;

	Block datablock = d.buildDataBlock();
	d.printBlock(datablock);
	cout << "data blocks after adding check bits :" << endl;
	Block parityBlock = d.buildBlockWithParity(datablock);
	cout << "data bits after column-wise serialization :" << endl;
	Block colWiseBlock = d.serializeColWise(parityBlock);
	d.printBlock(colWiseBlock);
	cout << "data bits after appending CRC checksum (sent fram) :" << endl;
	string crcString = d.appendCRCcheckSum(colWiseBlock);
	bool isFlipped[crcString.length()] = {0};
	for (int i = 0; i < crcString.length(); i++)
	{
		double rnd = randGen() / static_cast<double>(RAND_MAX);
		if (rnd < d.p)
		{
			crcString[i] = crcString[i] == '0' ? '1' : '0';
			isFlipped[i] = 1;
		}
	}
	cout << endl;
	cout << "received frame : " << endl;
	for (int i = 0; i < crcString.length(); i++)
	{
		if (isFlipped[i])
			cout << FRED(crcString[i]);
		else
			cout << crcString[i];
	}
	cout << endl;

	bool isError = d.computeCheckSum(crcString);
	cout << "result of CRC cheksum matching : ";
	if (isError)
	{
		cout << "error detected" << endl;
	}
	else
	{
		cout << "no error" << endl;
	}

	cout << "data block after removing CRC checksum bits : " << endl;
	Block crcFreBlock = d.buildCRCfreeBlock(crcString, isFlipped);

	cout << endl;
	cout << "data block after removing check bits : " << endl;
	Block checkFreBlock = d.buildParityFreeBlock(crcFreBlock);
	d.printBlock(checkFreBlock);

	cout << "output frame : ";
	cout << d.decodeBlock(checkFreBlock) << endl;
}