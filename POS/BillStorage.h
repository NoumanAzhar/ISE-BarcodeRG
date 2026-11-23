#pragma once
#include<optional>
#include<string>
#include<vector>
#include"Bill.h"
class IBillRepository {
public:
	virtual ~IBillRepository() = default;
	virtual bool save(const Bill& bill) = 0;
	virtual bool replace(const Bill& bill) = 0;
	virtual std::optional<Bill> findById(const std::string& billId) const = 0;
	virtual std::optional<Bill> findByOrderId(const std::string& orderId) const = 0;
	virtual std::vector<Bill> all() const = 0;
	virtual std::string nextId() const = 0;
};
class FileBillRepository : public IBillRepository{
	std::string billFile, detailFile;
	mutable std::vector<std::string> warningList;
public:
	FileBillRepository(std::string billFile, std::string detailFile);
	bool save(const Bill& bill)override;
	bool replace(const Bill& bill)override;
	std::optional<Bill> findById(const std::string& billId) const override;
	std::optional<Bill> findByOrderId(const std::string& orderId) const override;
	std::vector<Bill> all() const override;
	std::string nextId() const override;
	const std::vector<std::string>& warnings() const;
};
