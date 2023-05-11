#define  _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "Product.h"
#include "General.h"
#include "fileHelper.h"


#define MIN_DIG 3
#define MAX_DIG 5

void	initProduct(Product* pProduct)
{
	initProductNoBarcode(pProduct);
	getBorcdeCode(pProduct->barcode);
}

void	initProductNoBarcode(Product* pProduct)
{
	initProductName(pProduct);
	pProduct->type = getProductType();
	pProduct->price = getPositiveFloat("Enter product price\t");
	pProduct->count = getPositiveInt("Enter product number of items\t");
}

void initProductName(Product* pProduct)
{
	do {
		printf("enter product name up to %d chars\n", NAME_LENGTH );
		myGets(pProduct->name, sizeof(pProduct->name),stdin);
	} while (checkEmptyString(pProduct->name));
}

void	printProduct(const Product* pProduct)
{
	printf("%-20s %-10s\t", pProduct->name, pProduct->barcode);
	printf("%-20s %5.2f %10d\n", typeStr[pProduct->type], pProduct->price, pProduct->count);
}

int		saveProductToFile(const Product* pProduct, FILE* fp)
{
	if (fwrite(pProduct, sizeof(Product), 1, fp) != 1)
	{
		puts("Error saving product to file\n");
		return 0;
	}
	return 1;
}

int		loadProductFromFile(Product* pProduct, FILE* fp)
{
	if (fread(pProduct, sizeof(Product), 1, fp) != 1)
	{
		puts("Error reading product from file\n");
		return 0;
	}
	return 1;
}

void getBorcdeCode(char* code)
{
	char temp[MAX_STR_LEN];
	char msg[MAX_STR_LEN];
	sprintf(msg,"Code should be of %d length exactly\n"
				"UPPER CASE letter and digits\n"
				"Must have %d to %d digits\n"
				"First and last chars must be UPPER CASE letter\n"
				"For example A12B40C\n",
				BARCODE_LENGTH, MIN_DIG, MAX_DIG);
	int ok = 1;
	int digCount = 0;
	do {
		ok = 1;
		digCount = 0;
		printf("Enter product barcode "); 
		getsStrFixSize(temp, MAX_STR_LEN, msg);
		if (strlen(temp) != BARCODE_LENGTH)
		{
			puts(msg);
			ok = 0;
		}
		else {
			//check and first upper letters
			if(!isupper(temp[0]) || !isupper(temp[BARCODE_LENGTH-1]))
			{
				puts("First and last must be upper case letters\n");
				ok = 0;
			} else {
				for (int i = 1; i < BARCODE_LENGTH - 1; i++)
				{
					if (!isupper(temp[i]) && !isdigit(temp[i]))
					{
						puts("Only upper letters and digits\n");
						ok = 0;
						break;
					}
					if (isdigit(temp[i]))
						digCount++;
				}
				if (digCount < MIN_DIG || digCount > MAX_DIG)
				{
					puts("Incorrect number of digits\n");
					ok = 0;
				}
			}
		}
		
	} while (!ok);

	strcpy(code, temp);
}


eProductType getProductType()
{
	int option;
	printf("\n\n");
	do {
		printf("Please enter one of the following types\n");
		for (int i = 0; i < eNofProductType; i++)
			printf("%d for %s\n", i, typeStr[i]);
		scanf("%d", &option);
	} while (option < 0 || option >= eNofProductType);
	getchar();
	return (eProductType)option;
}

const char* getProductTypeStr(eProductType type)
{
	if (type < 0 || type >= eNofProductType)
		return NULL;
	return typeStr[type];
}

int		isProduct(const Product* pProduct, const char* barcode)
{
	if (strcmp(pProduct->barcode, barcode) == 0)
		return 1;
	return 0;
}

int		compareProductByBarcode(const void* var1, const void* var2)
{
	const Product* pProd1 = (const Product*)var1;
	const Product* pProd2 = (const Product*)var2;

	return strcmp(pProd1->barcode, pProd2->barcode);
}


void	updateProductCount(Product* pProduct)
{
	int count;
	do {
		printf("How many items to add to stock?");
		scanf("%d", &count);
	} while (count < 1);
	pProduct->count += count;
}


void	freeProduct(Product* pProduct)
{
	//nothing to free!!!!
}

int writeProductToCompressFile(Product* prod, FILE* fp)
{
	
	char barcode[BARCODE_LENGTH];
	strncpy(barcode, prod->barcode, BARCODE_LENGTH);
	int len = (int)strlen(prod->name);
	BYTE data1[3] = { 0 };
	BYTE data2[3] = { 0 };
	for (int i = 0; i < BARCODE_LENGTH; i++)
	{
		if (barcode[i] <= '9' && barcode[i] >= '0')
			barcode[i] = barcode[i]-'0';
		if (barcode[i] <= 'Z' && barcode[i] >= 'A')
			barcode[i] = barcode[i] - 'A' + 10;
	}
	data1[0] =barcode[0]  << 2 | barcode[1] >> 4;
	data1[1] = barcode[1] << 4 | barcode[2] >> 2;
	data1[2] = barcode[2] << 6 | barcode[3];

	data2[0] = barcode[4] << 2 | barcode[5] >> 4;
	data2[1] = barcode[5] << 4 | barcode[6] >> 2;
	data2[2] = barcode[6] << 6 | len << 2 | prod->type;

	if (fwrite(&data1, sizeof(BYTE), 3, fp) != 3) {
		return 0;
	}

	if (fwrite(&data2, sizeof(BYTE), 3, fp) != 3) {
		return 0;
	}
	if (fwrite(&prod->name, sizeof(char), len, fp) != len) {
		return 0;
	}
	BYTE data3[3] = { 0 };
	int decimalPart = (int)((prod->price - (int)prod->price) * 100);
	int num = (int)prod->price;

	data3[0] = prod->count;
	data3[1] = decimalPart << 1 | num >> 8;
	data3[2] = num;
	if (fwrite(&data3, sizeof(BYTE), 3, fp) != 3) {
		return 0;
	}
	return 1;
}

int readProductToCompressFile(Product* prod, FILE* fp)
{
	char barcode[BARCODE_LENGTH+1];
	int len;
	BYTE data1[3];
	BYTE data2[3];
	if (fread(&data1, sizeof(BYTE), 3, fp) != 3) {
		return 0;
	}
	
	if (fread(&data2, sizeof(BYTE), 3, fp) != 3) {
		return 0;
	}

	barcode[0] = (data1[0] >> 2) & 0x3F;
	barcode[1] = ((data1[1] >> 4) & 0xF) | (data1[0] << 4) & 0x30;
	barcode[2] = (data1[1] << 2) & 0x3C | (data1[2] >> 6) & 0x3;
	barcode[3] = data1[2] & 0x1F;

	barcode[4] = (data2[0] >> 2) & 0x3F;
	barcode[5] = ((data2[1] >> 4) & 0xF) | (data2[0] << 4) & 0x30;
	barcode[6] = (data2[1] << 2) &0x3C | (data2[2] >> 6) & 0x3;
	

	for (int i = 0; i < BARCODE_LENGTH; i++)
	{
		if (barcode[i] <= 9 && barcode[i] >= 0)
			barcode[i] = barcode[i] + '0';
		if (barcode[i] <= 35 && barcode[i] >= 10)
			barcode[i] = barcode[i] + 'A' - 10;
	}
	barcode[BARCODE_LENGTH] = '\0';
	strcpy(prod->barcode, barcode);

	len = (data2[2] >> 2) & 0xF;
	prod->type = data2[2] & 0x3;
	if (fread(&prod->name, sizeof(char), len, fp) != len) {
		return 0;
	}
	prod->name[len] = '\0';

	BYTE data3[3] = { 0 };
	int decimalPart;
	int num;
	if (fread(&data3, sizeof(BYTE), 3, fp) != 3) {
		return 0;
	}
	prod->count = data3[0];
	decimalPart = (data3[1] >> 1)&0x7F;
	num = data3[2] | (data3[1] << 8) & 0x100;
	prod->price = num + (float)decimalPart / 100;

	return 1;
}


