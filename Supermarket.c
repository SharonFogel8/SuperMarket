#define  _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Supermarket.h"
#include "Product.h"
#include "Customer.h"
#include "Address.h"
#include "General.h"
#include "ShoppingCart.h"
#include "GeneralList.h"
#include "SuperFile.h"
#include "myMacros.h"

int		initSuperMarket(SuperMarket* pMarket, const char* fileName,
					const char* customersFileName,int option)
{
	pMarket->customerCount = 0;
	pMarket->customerArr = NULL;
	pMarket->sortOpt = eNone;
	L_init(&pMarket->productList);
	if (option == 0) {
		if (loadSuperMarketFromFile(pMarket, fileName, customersFileName) == 1)
		{
			printf("Supermarket successfully loaded from files\n");
			return 1;
		}
	}
	else if (option == 1) {
		if (readCompressSuper(fileName, pMarket, customersFileName) == 1)
		{
			printf("Supermarket successfully loaded from files\n");
			return 1;
		}
	}
	

	pMarket->name = getStrExactLength("Enter market name");
	return initAddress(&pMarket->location);
}

void	printSuperMarket(const SuperMarket* pMarket)
{
	printf("Super Market Name: %s\t", pMarket->name);
	printf("Address: ");
	printAddress(&pMarket->location);
	printf("\n");
	printAllProducts(pMarket);
	printf("\n");
	printAllCustomers(pMarket);
	printf("\n");
}

int		addProduct(SuperMarket* pMarket)
{
	char barcode[BARCODE_LENGTH + 1];
	Product* pProd = getProductFromUser(pMarket,barcode);
	if (pProd != NULL) //This barcode exist in stock
		updateProductCount(pProd);
	else { //new product to stock
		if(!addNewProduct(pMarket, barcode))
			return 0;
	}

	return 1;
}

int		addNewProduct(SuperMarket* pMarket,const char* barcode)
{
	Product* pProd = (Product*)calloc(1, sizeof(Product));
	if (!pProd)
	{
		return 0;
	}

	strcpy(pProd->barcode, barcode);
	initProductNoBarcode(pProd);
	
	
	return insertNewProductToList(&pMarket->productList,pProd);
}

int		insertNewProductToList(LIST* pProductList, Product* pProduct)
{
	NODE* pN = pProductList->head.next; //first Node
	NODE* pPrevNode = &pProductList->head;
	Product* pTemp;
	int compRes;
	while (pN != NULL)
	{
		pTemp = (Product*)pN->key;
		compRes = strcmp(pTemp->barcode, pProduct->barcode);
		if (compRes == 0) //found a product with this name is cart
		{
			printf("Not new product\n");
			return 0;
		}

		if (compRes > 0) {//found a place for new item, the next one bigger	
			if(!L_insert(pPrevNode, pProduct))
				return 0;
			return 1;
		}
		pPrevNode = pN;
		pN = pN->next;
	}
	//insert at end
	if(!L_insert(pPrevNode, pProduct))
		return 0;
	return 1;
}

int		addCustomer(SuperMarket* pMarket)
{
	Customer cust;
	if (!initCustomer(&cust))
		return 0;

	if (isCustomerInMarket(pMarket, &cust))
	{
		printf("This customer already in market\n");
		free(cust.name);
		return 0;
	}

	pMarket->customerArr = (Customer*)realloc(pMarket->customerArr, (pMarket->customerCount + 1) * sizeof(Customer));
	if (!pMarket->customerArr)
	{
		free(cust.name);
		return 0;
	}

	pMarket->customerArr[pMarket->customerCount] = cust;
	pMarket->customerCount++;
	pMarket->sortOpt = eNone;
	return 1;
}

int		isCustomerInMarket(SuperMarket* pMarket, Customer* pCust)
{
	for (int i = 0; i < pMarket->customerCount; i++)
	{
		if (strcmp(pMarket->customerArr[i].name, pCust->name) == 0)
			return 1;
	}
	return 0;
}

int	doShopping(SuperMarket* pMarket)
{
	Customer* pCustomer = getCustomerShopPay(pMarket);
	if (!pCustomer)
		return 0;

	if (pCustomer->pCart == NULL)
	{
		pCustomer->pCart = (ShoppingCart*)malloc(sizeof(ShoppingCart));
		CHECK_RETURN_0(pCustomer->pCart);
		/*if (!pCustomer->pCart)
			return 0;*/
		initCart(pCustomer->pCart);
	}
	fillCart(pMarket, pCustomer->pCart);
	if (pCustomer->pCart->count == 0) //did not buy any thing
	{
		free(pCustomer->pCart);
		pCustomer->pCart = NULL;
	}
	printf("---------- Shopping ended ----------\n");
	return 1;
}

void handleCustomerStillShoppingAtExit(SuperMarket* pMarket)
{
	Customer* pCust;
	for (int i = 0; i < pMarket->customerCount; i++)
	{
		pCust = &pMarket->customerArr[i];
		if (pCust->pCart)
		{
			printf("Market is closing must pay!!!\n");
			pay(pCust); //will free every thing and update shope info
		}
	}
}

Customer*	doPrintCart(SuperMarket* pMarket)
{
	Customer* pCustomer = getCustomerShopPay(pMarket);
	if (!pCustomer)
		return NULL;
	if (pCustomer->pCart == NULL)
	{
		printf("Customer cart is empty\n");
		return NULL;
	}
	printShoppingCart(pCustomer->pCart);
	return pCustomer;
}

int	doPayment(SuperMarket* pMarket)
{
	Customer* pCustomer = doPrintCart(pMarket);
	CHECK_RETURN_0(pCustomer);
	/*if(!pCustomer)
		return 0;*/
	pay(pCustomer);
	return 1;
}

Customer*	getCustomerShopPay(SuperMarket* pMarket)
{
	if (pMarket->customerCount == 0)
	{
		printf("No customer listed to market\n");
		return NULL;
	}

	if (pMarket->productList.head.next == NULL)
	{
		printf("No products in market - cannot shop\n");
		return NULL;
	}

	Customer* pCustomer = getCustomerWhoShop(pMarket);
	if (!pCustomer)
	{
		printf("this customer not listed\n");
		return NULL;
	}

	return pCustomer;
}

void	printAllProducts(const SuperMarket* pMarket)
{
	printf("There are %d products\n", getNumOfProductsInList(pMarket));
	printf("%-20s %-10s\t", "Name", "Barcode");
	printf("%-20s %-10s %s\n", "Type", "Price", "Count In Stoke");
	printf("--------------------------------------------------------------------------------\n");
	L_print(&pMarket->productList, printProduct);
	/*for (int i = 0; i < pMarket->productCount; i++)
		printProduct(pMarket->productArr[i]);*/
}

void	printAllCustomers(const SuperMarket* pMarket)
{
	printf("There are %d listed customers\n", pMarket->customerCount);
	generalArrayFunction((void*)pMarket->customerArr, pMarket->customerCount, sizeof(Customer),
		printCustomer);
}


Customer* getCustomerWhoShop(SuperMarket* pMarket)
{
	printAllCustomers(pMarket);
	char name[MAX_STR_LEN];
	getsStrFixSize(name, sizeof(name), "Who is shopping? Enter cutomer name\n");

	Customer* pCustomer = FindCustomerByName(pMarket, name);
	
	return pCustomer;
}


void fillCart(SuperMarket* pMarket,ShoppingCart* pCart)
{
	printAllProducts(pMarket);
	char op;
	while (1)
	{
		printf("Do you want to shop for a product? y/Y, anything else to exit!!\t");
		do {
			scanf("%c", &op);
		} while (isspace(op));
		getchar(); //clean the enter
		if (op != 'y' && op != 'Y')
			break;
		int count;
		Product* pProd = getProductAndCount(pMarket, &count);
		if(pProd)
		{
			if (!addItemToCart(pCart, pProd->barcode, pProd->price, count))
			{
				printf("Error adding item\n");
				return;
			}
			pProd->count -= count; //item bought!!!
		}
	}
}

eSortOption showSortMenu()
{
	int opt;
	printf("Base on what field do you want to sort?\n");
	do {
		for (int i = 1; i < eNofSortOpt; i++)
			printf("Enter %d for %s\n", i, sortOptStr[i]);
		scanf("%d", &opt);
	} while (opt < 0 || opt >eNofSortOpt);

	return (eSortOption)opt;
}

void sortCustomers(SuperMarket * pMarket)
{
	pMarket->sortOpt = showSortMenu();
	int(*compare)(const void* air1, const void* air2) = NULL;

	compare = getCompareFunction(pMarket->sortOpt);

	if (compare != NULL)
		qsort(pMarket->customerArr, pMarket->customerCount, sizeof(Customer), compare);
	else
		printf("Error in sorting\n");
}

void findCustomer(const SuperMarket * pMarket)
{
	int(*compare)(const void* air1, const void* air2) = NULL;
	Customer cust = { 0 };

	compare = getCompareFunction(pMarket->sortOpt);


	switch (pMarket->sortOpt)
	{
	case eName:
		cust.name=getStrExactLength("Enter customer name\n");
		break;

	case eTime:
		printf("Enter time in matket\n");
		scanf("%d", &cust.shopTimes);
		break;

	case eSpend:
		printf("Enter spent amount\n");
		scanf("%f", &cust.totalSpend);
		break;
	}

	if (compare != NULL)
	{
		Customer* pTemp = bsearch(&cust,pMarket->customerArr, pMarket->customerCount, sizeof(Customer), compare);
		if (pTemp == NULL)
			printf("Customer was not found\n");
		else {
			printf("Customer found, ");
			printCustomer(pTemp);
		}
	}
	else {
		printf("The search cannot be performed, array not sorted\n");
	}
}


void* getCompareFunction(eSortOption sort)
{
	int(*compare)(const void* air1, const void* air2) = NULL;

	switch (sort)
	{
	case eName:
		compare = compareCustomerByName;
		break;
	case eTime:
		compare = compareCustomerByShopTime;
		break;
	case eSpend:
		compare = compareCustomerBySpent;
		break;

	}
	return compare;

}

int		getNumOfProductsInList(const SuperMarket* pMarket)
{
	int count = 0;
	NODE* pN = pMarket->productList.head.next; //first Node
	
	while (pN != NULL)
	{
		count++;
		pN = pN->next;
	}
	return count;
}

Product* getProductAndCount(SuperMarket* pMarket, int* pCount)
{
	char barcode[BARCODE_LENGTH + 1];
	Product* pProd = getProductFromUser(pMarket, barcode);
	if (pProd == NULL)
	{
		printf("No such product\n");
		return NULL;
	} 
	
	if (pProd->count == 0)
	{
		printf("This product out of stock\n");
		return NULL;
	}
	
	int count;
	do {
		printf("How many items do you want? max %d\n", pProd->count);
		scanf("%d", &count);
	} while (count <= 0 || count > pProd->count);
	*pCount = count;
	return pProd;
}

void	printProductByType(SuperMarket* pMarket)
{
	NODE* pN = pMarket->productList.head.next; //first Node
	if (!pN)
	{
		printf("No products in market\n");
		return;
	}
	eProductType	type = getProductType();
	int count = 0;

	Product* pTemp;
	while (pN != NULL)
	{
		pTemp = (Product*)pN->key;
		if (pTemp->type == type)
		{
			count++;
			printProduct(pTemp);
		}
		pN = pN->next;
	}
	if (count == 0)
		printf("There are no product of type %s in market %s\n", getProductTypeStr(type), pMarket->name);
}

Product* getProductFromUser(SuperMarket* pMarket, char* barcode)
{
	getBorcdeCode(barcode);
	return getProductByBarcode(pMarket, barcode);
}

void	freeMarket(SuperMarket* pMarket)
{
	free(pMarket->name);
	freeAddress(&pMarket->location);
	L_free(&pMarket->productList, freeProduct);
	
	generalArrayFunction((void*)pMarket->customerArr, pMarket->customerCount, sizeof(Customer),
		freeCustomer);

	free(pMarket->customerArr);
}

int writeCompressSuper(const SuperMarket* pMarket,FILE* fp)
{
	
	BYTE data[2] = { 0 };
	int numOfProduct = getNumOfProductsInList(pMarket);
	int nameLen = (int)strlen(pMarket->name);
	data[0] = numOfProduct >> 2;
	data[1] = numOfProduct << 6| nameLen;
	if (fwrite(&data, sizeof(BYTE), 2, fp) != 2) {
		return 0;
	}
	if (fwrite(pMarket->name, sizeof(char), nameLen, fp) != nameLen) {
		return 0;
	}
	if (!writeCompressAddress(&pMarket->location,fp))
		return 0;

	if (!writeCompressProducts(pMarket,numOfProduct, fp))
		return 0;
	
	fclose(fp);
	return 1;

}
int readCompressSuper(const char* fileName,SuperMarket* pMarket,const char* customersFileName)
{
	FILE* fp;
	fp = fopen(fileName, "rb");
	if (!fp) {
		return 0;
	}

	BYTE data[2];
	if (fread(&data, sizeof(BYTE), 2, fp) != 2) {
		fclose(fp);
		return 0;
	}
		
	
	int numOfProduct = data[0] << 2 | ((data[1] >> 6) & 0x3);
	int nameLen = data[1] & 0x3F;
	pMarket->name = (char*)calloc(nameLen+1,sizeof(char));	
	if (!pMarket->name) {
		fclose(fp);
		return 0;
	}
	if (fread(pMarket->name, sizeof(char), nameLen, fp) != nameLen) {
		FREE_CLOSE_FILE_RETURN_0(pMarket->name, fp);
		/*free(pMarket->name);
		fclose(fp);
		return 0;*/
	}

	if (!readCompressAddress(&pMarket->location, fp))
		return 0;


	if (!readCompressProducts(pMarket, numOfProduct, fp))
		return 0;
	fclose(fp);

	pMarket->customerArr = loadCustomerFromTextFile(customersFileName, &pMarket->customerCount);
	if (!pMarket->customerArr)
		return 0;

	return	1;

	return 1;

}

int writeCompressProducts(const SuperMarket* pMarket,int numOfProduct, FILE* fp)
{
	NODE* pN = pMarket->productList.head.next;
	for (int i = 0; i < numOfProduct; i++)
	{
		Product* prod = (Product*)pN->key;
		if (!writeProductToCompressFile(prod, fp))
			return 0;
		pN = pN->next;
	}
	return 1;
}
int readCompressProducts(SuperMarket* pMarket, int numOfProduct, FILE* fp)
{
	if (numOfProduct > 0)
	{
		Product* pTemp;
		for (int i = 0; i < numOfProduct; i++)
		{
			pTemp = (Product*)calloc(1, sizeof(Product));
			if (!pTemp)
			{
				printf("Allocation error\n");
				L_free(&pMarket->productList, freeProduct);
				free(pMarket->name);
				fclose(fp);
				return 0;
			}
			if (!readProductToCompressFile(pTemp, fp))
			{
				L_free(&pMarket->productList, freeProduct);
				free(pMarket->name);
				fclose(fp);
				return 0;
			}
			if (!insertNewProductToList(&pMarket->productList, pTemp))
			{
				L_free(&pMarket->productList, freeProduct);
				free(pMarket->name);
				fclose(fp);
				return 0;
			}
		}
	}
	return 1;
}

void	getUniquBarcode(char* barcode, SuperMarket* pMarket)
{
	int cont = 1;
	Product* pTemp;
	while (cont)
	{
		getBorcdeCode(barcode);
		pTemp = getProductByBarcode(pMarket, barcode);
		if (pTemp == NULL)
			cont = 0;
		else
			printf("This product already in market\n");
	}
}

Product* getProductByBarcode(SuperMarket* pMarket, const char* barcode)
{
	Product temp;
	strcpy(temp.barcode, barcode);
	NODE* pN = L_find(pMarket->productList.head.next,&temp,compareProductByBarcode);

	if (pN != NULL)
		return (Product*)pN->key;

	return NULL;
}

Customer* FindCustomerByName(SuperMarket* pMarket, const char* name)
{
	for (int i = 0; i < pMarket->customerCount; i++)
	{
		if (isCustomer(&pMarket->customerArr[i], name))
			return &pMarket->customerArr[i];
	}
	return  NULL;
}
