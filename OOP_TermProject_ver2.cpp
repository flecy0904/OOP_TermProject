// == 1. 헤더 및 전방 선언 ==
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace std;

// 전방 선언 (순환 참조 해결)
class Stock;
class Market;

// == 2. Enum 및 상수 정의 ==

enum OrderType
{
    BUY,
    SELL
};
enum PriceType
{
    MARKET,
    LIMIT
};
enum OrderStatus
{
    PENDING,
    COMPLETED,
    CANCELLED
};

// 상수 정의
const double DEFAULT_FEE_RATE = 0.00015;      // 0.015% 수수료
const double DEFAULT_PANIC_THRESHOLD = -0.10; // -10% 손절
const double DEFAULT_DCA_DROP_RATE = -0.05;   // -5% 물타기
const int DEFAULT_DCA_INTERVAL = 5;           // 5일 간격
const double DEFAULT_DCA_BUY_RATIO = 0.25;    // 25% 매수
const double DEFAULT_HOLD_BUY_RATIO = 0.5;    // 50% 매수
const long DEFAULT_INITIAL_CASH = 10000000;   // 1000만원

// == 3. 구조체 정의 ==

// 3.2 BacktestConfig 구조체
struct BacktestConfig
{
    long initialCash;
    double feeRate;
    double panicThreshold;
    double dcaDropRate;
    int dcaInterval;
    double dcaBuyRatio;
    double holdBuyRatio;

    // 기본값 생성자
    BacktestConfig() : initialCash(DEFAULT_INITIAL_CASH),
                       feeRate(DEFAULT_FEE_RATE),
                       panicThreshold(DEFAULT_PANIC_THRESHOLD),
                       dcaDropRate(DEFAULT_DCA_DROP_RATE),
                       dcaInterval(DEFAULT_DCA_INTERVAL),
                       dcaBuyRatio(DEFAULT_DCA_BUY_RATIO),
                       holdBuyRatio(DEFAULT_HOLD_BUY_RATIO) {}
};

// 3.3 StrategyReport 구조체
struct StrategyReport
{
    string strategyName; // 전략 이름
    long initialCash;    // 초기 자본
    long finalEquity;    // 최종 자산
    double totalReturn;  // 총 수익률 (%)
    double maxDrawdown;  // 최대 낙폭 (%)
    int buyCount;        // 매수 횟수
    int sellCount;       // 매도 횟수
    int finalShares;     // 최종 보유 주식 수
    int avgPrice;        // 평균 매수가
};

// == 4. 기본 클래스 설계 ==

// 4.1 Stock 클래스
class Stock
{
private:
    string code;
    string name;
    int currentPrice;
    int previousPrice;
    vector<int> priceHistory;

public:
    Stock(string c, string n, int p) : code(c), name(n), currentPrice(p), previousPrice(p) {}

    void updatePrice(int newPrice)
    {
        previousPrice = currentPrice;
        currentPrice = newPrice;
    }

    void addPriceHistory(int price)
    {
        priceHistory.push_back(price);
    }

    double getChangeRate() const
    {
        if (previousPrice == 0)
            return 0.0;
        return (double)(currentPrice - previousPrice) / previousPrice * 100.0;
    }

    int getPriceAt(size_t idx) const
    {
        if (idx >= priceHistory.size())
            return -1;
        return priceHistory[idx];
    }

    size_t getHistoryLength() const
    {
        return priceHistory.size();
    }

    string getCode() const { return code; }
    string getName() const { return name; }
    int getCurrentPrice() const { return currentPrice; }
};

// 4.2 Position 클래스
class Position
{
private:
    Stock *stock;
    int quantity;
    int avgPrice;
    long totalInvested;

public:
    Position(Stock *s = nullptr, int qty = 0, int price = 0)
        : stock(s), quantity(qty), avgPrice(price), totalInvested(0)
    {
        if (qty > 0)
        {
            totalInvested = (long)qty * price;
        }
    }

    void addQuantity(int qty, int price)
    {
        totalInvested += (long)qty * price;
        quantity += qty;
        avgPrice = (quantity > 0) ? (int)(totalInvested / quantity) : 0;
    }

    bool reduceQuantity(int qty)
    {
        if (qty > quantity)
            return false;
        quantity -= qty;
        // 매도 시에는 평균단가가 변하지 않고 totalInvested 비례 감소
        if (quantity == 0)
        {
            totalInvested = 0;
            avgPrice = 0;
        }
        else
        {
            totalInvested = (long)quantity * avgPrice;
        }
        return true;
    }

    long getCurrentValue() const
    {
        if (!stock)
            return 0;
        return (long)stock->getCurrentPrice() * quantity;
    }

    long getProfit() const
    {
        return getCurrentValue() - totalInvested;
    }

    double getProfitRate() const
    {
        if (totalInvested == 0)
            return 0.0;
        return (double)getProfit() / totalInvested * 100.0;
    }

    int getQuantity() const { return quantity; }
    int getAvgPrice() const { return avgPrice; }
    Stock *getStock() const { return stock; }
};

// 4.3 Portfolio 클래스
class Portfolio
{
private:
    map<string, Position> positions;

public:
    void addPosition(Stock *s, int qty, int price)
    {
        string code = s->getCode();
        if (positions.find(code) != positions.end())
        {
            positions[code].addQuantity(qty, price);
        }
        else
        {
            positions[code] = Position(s, qty, price);
        }
    }

    bool reducePosition(string code, int qty)
    {
        if (positions.find(code) == positions.end())
            return false;

        bool result = positions[code].reduceQuantity(qty);
        if (result && positions[code].getQuantity() == 0)
        {
            positions.erase(code);
        }
        return result;
    }

    Position *getPosition(string code)
    {
        if (positions.find(code) == positions.end())
            return nullptr;
        return &positions[code];
    }

    bool hasPosition(string code) const
    {
        return positions.find(code) != positions.end();
    }

    long getTotalValue() const
    {
        long total = 0;
        for (const auto &entry : positions)
        {
            const string &code = entry.first;
            const Position &pos = entry.second;
            total += pos.getCurrentValue();
        }
        return total;
    }

    long getTotalProfit() const
    {
        long total = 0;
        for (const auto &entry : positions)
        {
            const string &code = entry.first;
            const Position &pos = entry.second;
            total += pos.getProfit();
        }
        return total;
    }

    void printPortfolio() const
    {
        cout << "=== 보유 종목 현황 ===" << endl;
        for (const auto &entry : positions)
        {
            const string &code = entry.first;
            const Position &pos = entry.second;
            cout << "[" << code << "] " << pos.getStock()->getName()
                 << " | 수량: " << pos.getQuantity()
                 << " | 평단: " << pos.getAvgPrice()
                 << " | 현재가: " << pos.getStock()->getCurrentPrice()
                 << " | 수익률: " << fixed << setprecision(2) << pos.getProfitRate() << "%" << endl;
        }
    }
};

// 4.4 Order 클래스
class Order
{
private:
    static int nextOrderId; // 정적 멤버 선언

    int orderId;
    string stockCode;
    OrderType orderType;
    PriceType priceType;
    int requestedPrice;
    int quantity;
    OrderStatus status;
    time_t timestamp;

public:
    Order(string code, OrderType ot, PriceType pt, int price, int qty)
        : orderId(nextOrderId++), stockCode(code), orderType(ot),
          priceType(pt), requestedPrice(price), quantity(qty),
          status(PENDING), timestamp(time(0)) {}

    void execute() { status = COMPLETED; }
    void cancel() { status = CANCELLED; }

    bool isPending() const { return status == PENDING; }
    int getOrderId() const { return orderId; }
    string getStockCode() const { return stockCode; }
    OrderType getOrderType() const { return orderType; }
    int getQuantity() const { return quantity; }

    void printOrder() const
    {
        string typeStr = (orderType == BUY) ? "매수" : "매도";
        string statusStr = (status == PENDING) ? "대기" : (status == COMPLETED ? "체결" : "취소");
        cout << "주문 #" << orderId << " [" << stockCode << "] " << typeStr
             << " " << quantity << "주 (" << statusStr << ")" << endl;
    }
};

// 정적 멤버 초기화
int Order::nextOrderId = 1;

// 4.5 Transaction 클래스
class Transaction
{
private:
    static int nextTransactionId;

    int transactionId;
    int orderId;
    string stockCode;
    string stockName;
    string type;
    int quantity;
    int price;
    long totalAmount;
    int fee;
    time_t timestamp;

public:
    Transaction(Order &order, Stock *stock, int execPrice)
        : transactionId(nextTransactionId++), orderId(order.getOrderId()),
          stockCode(stock->getCode()), stockName(stock->getName()),
          quantity(order.getQuantity()), price(execPrice), timestamp(time(0))
    {
        type = (order.getOrderType() == BUY) ? "BUY" : "SELL";
        totalAmount = (long)price * quantity;
        fee = (int)(totalAmount * DEFAULT_FEE_RATE);
    }

    long getNetAmount() const
    {
        if (type == "BUY")
            return totalAmount + fee; // 매수 시 수수료 더함
        else
            return totalAmount - fee; // 매도 시 수수료 차감 (입금액)
    }

    void printLog() const
    {
        cout << "거래 #" << transactionId << " [" << type << "] "
             << stockName << " " << quantity << "주 @ " << price << "원" << endl;
    }
};

int Transaction::nextTransactionId = 1;

// 4.6 Market 클래스 (전방 선언으로 인해 순서 조정)
class Market
{
private:
    vector<Stock *> stocks;

public:
    Market() {}
    ~Market()
    {
        // Market이 Stock 객체의 소유권을 가짐 (설계서 명시)
        for (Stock *s : stocks)
        {
            delete s;
        }
    }

    void addStock(Stock *stock)
    {
        stocks.push_back(stock);
    }

    Stock *getStock(string code)
    {
        for (Stock *s : stocks)
        {
            if (s->getCode() == code)
                return s;
        }
        return nullptr;
    }

    void simulatePriceChange()
    {
        for (Stock *stock : stocks)
        {
            int randVal = rand() % 100;
            double rate;

            if (randVal < 95)
            {
                // 95%: -3% ~ +3%
                rate = ((rand() % 601) - 300) / 10000.0;
            }
            else
            {
                // 5%: -15% ~ -5% 급락 (설계서 오타 수정 추정: 급등락 또는 급락 시나리오)
                // 명세서 수식: -((rand() % 1001) + 500) / 10000.0 -> -0.05 ~ -0.15
                rate = -((rand() % 1001) + 500) / 10000.0;
            }

            int newPrice = (int)(stock->getCurrentPrice() * (1 + rate));
            if (newPrice < 1)
                newPrice = 1;
            stock->updatePrice(newPrice);
        }
    }
};

// 4.6 Account 클래스
class Account
{
private:
    string accountNumber;
    long balance;
    Portfolio portfolio;
    vector<Order> orders;
    vector<Transaction> transactions;

public:
    Account(string accNum, long initBal) : accountNumber(accNum), balance(initBal) {}

    void deposit(long amount)
    {
        if (amount > 0)
            balance += amount;
    }

    bool withdraw(long amount)
    {
        if (balance >= amount)
        {
            balance -= amount;
            return true;
        }
        return false;
    }

    bool placeOrder(Order order)
    {
        // 간단한 유효성 검사 (실제 체결 시 다시 검사하지만 주문 접수 단계)
        orders.push_back(order);
        return true;
    }

    bool executeOrder(int orderId, Market &m)
    {
        for (auto &order : orders)
        {
            if (order.getOrderId() == orderId && order.isPending())
            {
                Stock *stock = m.getStock(order.getStockCode());
                if (!stock)
                    return false;

                int currentPrice = stock->getCurrentPrice();
                long totalCost = (long)currentPrice * order.getQuantity();
                int fee = (int)(totalCost * DEFAULT_FEE_RATE);

                if (order.getOrderType() == BUY)
                {
                    if (balance >= totalCost + fee)
                    {
                        balance -= (totalCost + fee);
                        portfolio.addPosition(stock, order.getQuantity(), currentPrice);
                        order.execute();
                        transactions.push_back(Transaction(order, stock, currentPrice));
                        return true;
                    }
                }
                else if (order.getOrderType() == SELL)
                {
                    if (portfolio.hasPosition(stock->getCode()))
                    {
                        Position *pos = portfolio.getPosition(stock->getCode());
                        if (pos && pos->getQuantity() >= order.getQuantity())
                        {
                            portfolio.reducePosition(stock->getCode(), order.getQuantity());
                            long netRevenue = totalCost - fee;
                            balance += netRevenue;
                            order.execute();
                            transactions.push_back(Transaction(order, stock, currentPrice));
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    long getBalance() const { return balance; }

    long getTotalAssetValue() const
    {
        return balance + portfolio.getTotalValue();
    }

    Portfolio &getPortfolio() { return portfolio; } // const 제거 (수정 가능 참조)

    void printAccountSummary() const
    {
        cout << "계좌번호: " << accountNumber << " | 예수금: " << balance
             << " | 총 자산: " << getTotalAssetValue() << endl;
    }
};

// 4.8 User 클래스
class User
{
private:
    string userId;
    string password;
    string name;
    Account *account; // User가 소유

public:
    User(string id, string pw, string n) : userId(id), password(pw), name(n)
    {
        // 계좌 자동 생성 (임의 번호)
        account = new Account(id + "_ACC", 0);
    }

    ~User()
    {
        delete account;
    }

    bool login(string id, string pw) const
    {
        return (userId == id && password == pw);
    }

    Account *getAccount()
    {
        return account;
    }
};

// == 5. 습관 교정 백테스터 클래스 ==

// 5.1 TradingStrategy 클래스 (추상)
class TradingStrategy
{
protected:
    string name;
    long cash;
    int shares;
    int avgPrice;
    vector<long> equityHistory;
    int buyCount;
    int sellCount;

    void buy(int price, int qty, double feeRate)
    {
        long cost = (long)price * qty;
        long fee = (long)(cost * feeRate);

        if (cash >= cost + fee && qty > 0)
        {
            long totalCost = (long)avgPrice * shares + cost;
            shares += qty;
            avgPrice = (shares > 0) ? (int)(totalCost / shares) : 0;
            cash -= (cost + fee);
            buyCount++;
        }
    }

    void sellAll(int price, double feeRate)
    {
        if (shares > 0)
        {
            long revenue = (long)price * shares;
            long fee = (long)(revenue * feeRate);
            cash += (revenue - fee);
            shares = 0;
            avgPrice = 0;
            sellCount++;
        }
    }

    void recordEquity(int price)
    {
        equityHistory.push_back(getTotalValue(price));
    }

public:
    TradingStrategy(string n, long initCash)
        : name(n), cash(initCash), shares(0), avgPrice(0), buyCount(0), sellCount(0) {}

    virtual ~TradingStrategy() {}

    virtual void onPrice(size_t idx, int price, double rate) = 0;

    virtual void onFinish(int lastPrice)
    {
        // 종료 시 별도 작업 없음
    }

    string getName() const { return name; }

    long getTotalValue(int price) const
    {
        return cash + (long)shares * price;
    }

    const vector<long> &getEquityHistory() const { return equityHistory; }
    int getBuyCount() const { return buyCount; }
    int getSellCount() const { return sellCount; }
    int getShares() const { return shares; }
    int getAvgPrice() const { return avgPrice; }
};

// 5.2 PanicSellStrategy 클래스 (쫄보)
class PanicSellStrategy : public TradingStrategy
{
private:
    double stopLossRate;
    double feeRate;
    bool hasBought;

public:
    PanicSellStrategy(long initCash, double threshold, double fee)
        : TradingStrategy("쫄보 (Panic Seller)", initCash),
          stopLossRate(threshold), feeRate(fee), hasBought(false) {}

    void onPrice(size_t idx, int price, double changeRate) override
    {
        // 1. 첫 시점에 전액 매수
        if (!hasBought && cash >= price)
        {
            int qty = cash / (price + (int)(price * feeRate));
            if (qty > 0)
            {
                buy(price, qty, feeRate);
                hasBought = true;
            }
        }
        // 2. 보유 중이면 손절 체크
        else if (shares > 0 && avgPrice > 0)
        {
            double profitRate = (double)(price - avgPrice) / avgPrice;
            if (profitRate <= stopLossRate)
            {
                sellAll(price, feeRate); // 공포 손절!
            }
        }
        // 3. 매 시점 자산 기록
        recordEquity(price);
    }
};

// 5.3 DCAStrategy 클래스 (코치)
class DCAStrategy : public TradingStrategy
{
private:
    double dcaDropRate;
    int dcaInterval;
    double buyRatio;
    double feeRate;
    int lastBuyIndex;
    int lastBuyPrice;

public:
    DCAStrategy(long initCash, double dropRate, int interval, double ratio, double fee)
        : TradingStrategy("코치 (DCA)", initCash),
          dcaDropRate(dropRate), dcaInterval(interval), buyRatio(ratio), feeRate(fee),
          lastBuyIndex(-1), lastBuyPrice(0) {}

    void onPrice(size_t idx, int price, double changeRate) override
    {
        bool shouldBuy = false;

        if (lastBuyIndex < 0 && cash >= price)
        {
            shouldBuy = true; // 첫 매수
        }
        else if (cash >= price)
        {
            bool intervalMet = ((int)idx - lastBuyIndex) >= dcaInterval;
            bool dropMet = (lastBuyPrice > 0) &&
                           ((double)(price - lastBuyPrice) / lastBuyPrice <= dcaDropRate);

            if (intervalMet || dropMet)
                shouldBuy = true;
        }

        if (shouldBuy)
        {
            long buyAmount = (long)(cash * buyRatio);
            // 최소 1주 이상 살 수 있는지 체크 (예외 처리 강화)
            if (buyAmount < price)
                buyAmount = cash; // 남은 돈이 적으면 전액

            int qty = buyAmount / (price + (int)(price * feeRate));
            if (qty > 0)
            {
                buy(price, qty, feeRate);
                lastBuyIndex = (int)idx;
                lastBuyPrice = price;
            }
        }
        recordEquity(price);
    }
};

// 5.4 HoldStrategy 클래스 (존버)
class HoldStrategy : public TradingStrategy
{
private:
    double initialBuyRatio;
    double feeRate;
    bool hasBought;

public:
    HoldStrategy(long initCash, double ratio, double fee)
        : TradingStrategy("존버 (Holder)", initCash),
          initialBuyRatio(ratio), feeRate(fee), hasBought(false) {}

    void onPrice(size_t idx, int price, double changeRate) override
    {
        if (!hasBought && cash >= price)
        {
            long buyAmount = (long)(cash * initialBuyRatio); // 명세에는 0.5 비율
            // 만약 initialBuyRatio가 1.0(전액)이 아니라면 잔고가 남음

            int qty = buyAmount / (price + (int)(price * feeRate));
            if (qty > 0)
            {
                buy(price, qty, feeRate);
                hasBought = true;
            }
        }
        recordEquity(price);
    }
};

// 5.5 BacktestEngine 클래스
class BacktestEngine
{
private:
    Stock *stock; // 외부 소유, delete 금지
    BacktestConfig config;
    vector<TradingStrategy *> strategies;
    vector<StrategyReport> results;

    StrategyReport buildReport(TradingStrategy *s, int lastPrice)
    {
        StrategyReport report;
        report.strategyName = s->getName();
        report.initialCash = config.initialCash;
        report.finalEquity = s->getTotalValue(lastPrice);
        report.totalReturn = (double)(report.finalEquity - report.initialCash) / report.initialCash * 100.0;
        report.maxDrawdown = calculateMDD(s->getEquityHistory());
        report.buyCount = s->getBuyCount();
        report.sellCount = s->getSellCount();
        report.finalShares = s->getShares();
        report.avgPrice = s->getAvgPrice();
        return report;
    }

public:
    BacktestEngine(Stock *s, const BacktestConfig &cfg) : stock(s), config(cfg) {}

    ~BacktestEngine()
    {
        for (auto s : strategies)
        {
            delete s;
        }
    }

    void addStrategy(TradingStrategy *s)
    {
        strategies.push_back(s);
    }

    double calculateMDD(const vector<long> &history)
    {
        if (history.empty())
            return 0.0;
        long peak = 0;
        double maxDD = 0.0;

        for (long equity : history)
        {
            if (equity > peak)
                peak = equity;
            if (peak > 0)
            {
                double dd = (double)(peak - equity) / peak;
                if (dd > maxDD)
                    maxDD = dd;
            }
        }
        return maxDD * 100.0; // % 반환
    }

    void runBattle()
    {
        size_t len = stock->getHistoryLength();
        if (len == 0)
            return;

        int prevPrice = stock->getPriceAt(0);

        for (size_t i = 0; i < len; ++i)
        {
            int price = stock->getPriceAt(i);
            double changeRate = (i == 0) ? 0.0 : (double)(price - prevPrice) / prevPrice * 100;

            for (TradingStrategy *s : strategies)
            {
                s->onPrice(i, price, changeRate);
            }
            prevPrice = price;
        }

        int lastPrice = stock->getPriceAt(len - 1);
        for (TradingStrategy *s : strategies)
        {
            s->onFinish(lastPrice);
            results.push_back(buildReport(s, lastPrice));
        }
    }

    const vector<StrategyReport> &getResults() const { return results; }
    Stock *getStock() const { return stock; }
};

// 5.6 BacktestReport 클래스
class BacktestReport
{
private:
    vector<StrategyReport> results;
    string stockName;

public:
    BacktestReport(const BacktestEngine &engine)
    {
        results = engine.getResults();
        stockName = engine.getStock()->getName();
    }

    void printSummary() const
    {
        cout << "\n=== 습관 교정 백테스터 결과 리포트 ===" << endl;
        cout << "종목: " << stockName << endl;
        if (results.empty())
            return;

        cout << "초기 자본: " << results[0].initialCash << "원" << endl
             << endl;

        for (const auto &res : results)
        {
            cout << "[" << res.strategyName << "]" << endl;
            cout << "최종 자산: " << res.finalEquity << "원 | 수익률: "
                 << fixed << setprecision(2) << res.totalReturn << "%" << endl;
            cout << "MDD: " << res.maxDrawdown << "% | 매수: " << res.buyCount
                 << "회 | 매도: " << res.sellCount << "회" << endl
                 << endl;
        }
    }

    void printRanking() const
    {
        // 간단한 수익률 순위 정렬 및 출력 (버블 정렬 사용, 데이터 작음)
        vector<StrategyReport> ranked = results;
        for (size_t i = 0; i < ranked.size(); ++i)
        {
            for (size_t j = 0; j < ranked.size() - 1 - i; ++j)
            {
                if (ranked[j].totalReturn < ranked[j + 1].totalReturn)
                {
                    swap(ranked[j], ranked[j + 1]);
                }
            }
        }

        cout << "순위: ";
        for (size_t i = 0; i < ranked.size(); ++i)
        {
            cout << (i + 1) << ". " << ranked[i].strategyName
                 << "(" << (ranked[i].totalReturn > 0 ? "+" : "")
                 << fixed << setprecision(0) << ranked[i].totalReturn << "%) ";
        }
        cout << endl;

        cout << "승자: " << ranked[0].strategyName << endl;
    }

    string getSummaryComment() const
    {
        if (results.size() < 2)
            return "결과 부족";

        // 전략 순서가 코드 삽입 순서와 같다고 가정 (0: 쫄보, 1: 코치, 2: 존버)
        // 명세서 상 비교 로직: DCA vs Panic

        // 전략 이름으로 인덱스 찾기
        int panicIdx = -1;
        int dcaIdx = -1;

        for (size_t i = 0; i < results.size(); ++i)
        {
            if (results[i].strategyName.find("쫄보") != string::npos)
                panicIdx = i;
            if (results[i].strategyName.find("코치") != string::npos)
                dcaIdx = i;
        }

        if (panicIdx == -1 || dcaIdx == -1)
            return "비교 대상 전략이 없습니다.";

        double panicReturn = results[panicIdx].totalReturn;
        double dcaReturn = results[dcaIdx].totalReturn;
        double diff = dcaReturn - panicReturn;

        stringstream ss;
        ss << fixed << setprecision(2);

        if (diff > 0)
        {
            ss << "물타기 전략이 감정적 손절 전략보다 " << diff << "%p 높은 수익을 기록했습니다.\n"
               << "뇌동매매를 줄이고 원칙을 지키는 습관을 길러보세요.";
        }
        else
        {
            ss << "이번 시나리오에서는 손절 전략이 유리했습니다.\n"
               << "하지만 장기적으로는 원칙 투자가 더 안정적입니다.";
        }
        return ss.str();
    }
};

// == 6. Main 함수 (실행 예시) ==

int main()
{
    srand((unsigned int)time(0));

    // 1. 시장 및 종목 생성
    Market market;
    Stock *samsung = new Stock("005930", "삼성전자", 70000);

    // 샘플 가격 데이터 (30일) - 명세서 섹션 8.1
    vector<int> prices = {
        70000, 71000, 69500, 68000, 65000, // 1-5일 (하락 시작)
        62000, 58000, 55000, 53000, 50000, // 6-10일 (급락)
        48000, 49000, 51000, 52000, 54000, // 11-15일 (바닥, 반등)
        56000, 58000, 60000, 62000, 64000, // 16-20일 (회복)
        65000, 67000, 68000, 70000, 72000, // 21-25일 (상승)
        74000, 75000, 76000, 78000, 80000  // 26-30일 (신고가)
    };

    for (int p : prices)
        samsung->addPriceHistory(p);
    market.addStock(samsung);

    // ==========================================
    // [TEST 1] 기본 거래 시스템 테스트
    // ==========================================
    cout << "=== [TEST 1] 기본 거래 기능 테스트 ===" << endl;

    // 2. 사용자 생성 및 입금
    User user("user1", "1234", "홍길동");
    Account *myAccount = user.getAccount();

    myAccount->deposit(10000000); // 1,000만원 입금
    myAccount->printAccountSummary();

    // 2.1 매수 테스트 (삼성전자 10주)
    cout << "\n>> [주문 1] 삼성전자 10주 매수 주문 (현재가: " << samsung->getCurrentPrice() << "원)" << endl;
    Order buyOrder("005930", BUY, MARKET, 0, 10);
    myAccount->placeOrder(buyOrder);

    // 주문 체결 시도
    if (myAccount->executeOrder(buyOrder.getOrderId(), market))
    {
        cout << "-> 체결 성공!" << endl;
    }
    else
    {
        cout << "-> 체결 실패" << endl;
    }

    // 2.2 [수정] Market 시뮬레이션을 통한 주가 변동
    cout << "\n-- Market 시뮬레이션 가동 (주가 랜덤 변동) --" << endl;
    market.simulatePriceChange();

    // 상태 확인 (수익률 변동 확인)
    myAccount->printAccountSummary();
    myAccount->getPortfolio().printPortfolio();

    // 2.3 매도 테스트 (삼성전자 5주)
    cout << "\n>> [주문 2] 삼성전자 5주 매도 주문 (현재가: " << samsung->getCurrentPrice() << "원)" << endl;
    Order sellOrder("005930", SELL, MARKET, 0, 5);
    myAccount->placeOrder(sellOrder);

    // 주문 체결 시도
    if (myAccount->executeOrder(sellOrder.getOrderId(), market))
    {
        cout << "-> 체결 성공! (차익 실현)" << endl;
    }
    else
    {
        cout << "-> 체결 실패" << endl;
    }

    // 최종 상태 확인
    myAccount->printAccountSummary();
    myAccount->getPortfolio().printPortfolio();
    cout << "------------------------------------------\n"
         << endl;

    // ==========================================
    // [TEST 2] 습관 교정 백테스터 실행
    // ==========================================
    cout << "=== [TEST 2] 습관 교정 백테스터 실행 ===" << endl;

    // 3. 백테스트 설정 및 엔진 생성
    BacktestConfig config;
    BacktestEngine engine(samsung, config);

    // 전략 추가
    // 3.1 쫄보 전략
    engine.addStrategy(new PanicSellStrategy(
        config.initialCash, config.panicThreshold, config.feeRate));

    // 3.2 코치 전략 (DCA)
    engine.addStrategy(new DCAStrategy(
        config.initialCash, config.dcaDropRate,
        config.dcaInterval, config.dcaBuyRatio, config.feeRate));

    // 3.3 존버 전략 (Hold)
    engine.addStrategy(new HoldStrategy(
        config.initialCash, config.holdBuyRatio, config.feeRate));

    // 배틀 시작
    engine.runBattle();

    // 4. 결과 출력
    BacktestReport report(engine);
    report.printSummary();
    report.printRanking();
    cout << endl
         << report.getSummaryComment() << endl;

    return 0;
}
