/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Ersohn Styne
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#ifndef CURRENCY_H
#define CURRENCY_H

#define CURRENCY_FINANCE_AUD "AU$" /* Australia       */
#define CURRENCY_FINANCE_BR  "R$"  /* Brazil          */
#define CURRENCY_FINANCE_CH  "₣"   /* Swiss           */
#define CURRENCY_FINANCE_EU  "€"   /* Europe          */
#define CURRENCY_FINANCE_GB  "£"   /* British         */
#define CURRENCY_FINANCE_HU  "Ft"  /* Hungary         */
#define CURRENCY_FINANCE_HKD "HK$" /* Chinese (HK)    */
#define CURRENCY_FINANCE_ID  "Rp." /* Indonesia       */
#define CURRENCY_FINANCE_IN  "₹"   /* India           */
#define CURRENCY_FINANCE_JA  "¥"   /* Japanese        */
#define CURRENCY_FINANCE_KR  "₩"   /* Korean          */
#define CURRENCY_FINANCE_NO  "kr"  /* Norwegian       */
#define CURRENCY_FINANCE_NZD "NZ$" /* New Zealand     */
#define CURRENCY_FINANCE_PL  "zł"  /* Polnish         */
#define CURRENCY_FINANCE_SG  "SG$" /* Singapore       */
#define CURRENCY_FINANCE_TH  "฿"   /* Thailand        */
#define CURRENCY_FINANCE_TWD "TW$" /* Chinese (TW)    */
#define CURRENCY_FINANCE_UAE "د.إ" /* UAE             */
#define CURRENCY_FINANCE_US  "$"   /* USA             */

void currency_init(void);
void currency_quit(void);
int currency_get_code_from_locale(const char  *locale,
                                  char       **currency_code,
                                  char       **currency_icon,
                                  int         *subunit);
const char *currency_get_price_from_locale(const char *, float);

#endif
